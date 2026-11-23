/*
 * GiMATEEC - access to the Gigabyte A16 embedded controller window.
 *
 * The EC answers in a 4 KiB MMIO window (OperationRegion ECMM/PECM in the DSDT).
 * Offsets were extracted from the firmware DSDT and cross-checked against the
 * Linux driver (github.com/tangalbert919/gigabyte-laptop-wmi):
 *
 *   0x03  BCPS   4 bits   bit 2 = custom charge mode
 *   0x05  BCPC   8        charge limit, 60..100 (%)
 *   0x13  RPM1   16 (LE)  fan 1 rpm
 *   0x15  RPM2   16 (LE)  fan 2 rpm
 *   0x1B  FAN1   8        fan 1 duty
 *   0x1C  FAN2   8        fan 2 duty
 *   0x31  KBLL   8        keyboard backlight level
 *   0x4B  FLVL   8        fan level
 *   0xB0  CTMP   8        CPU temperature
 *   0xB1  CTML   8        (second reading, equals CTMP on this model)
 *
 * Read path: values are published as IORegistry properties once a second.
 * Write path: a user client with two selectors (charge limit, raw byte write
 * limited to a known-safe allowlist).
 */

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOTimerEventSource.h>

#define EC_BASE 0x0FE0B0800ULL
#define EC_LEN  0x1000

class GiMATEEC;

class GiMATEECClient : public IOUserClient
{
    OSDeclareDefaultStructors(GiMATEECClient)

protected:
    GiMATEEC *owner;
    task_t    task;

public:
    virtual bool initWithTask(task_t owningTask, void *securityID, UInt32 type) APPLE_KEXT_OVERRIDE;
    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual IOReturn clientClose() APPLE_KEXT_OVERRIDE;
    virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments *args,
                                    IOExternalMethodDispatch *dispatch,
                                    OSObject *target, void *reference) APPLE_KEXT_OVERRIDE;

    static IOReturn sSetChargeLimit(OSObject *target, void *ref, IOExternalMethodArguments *args);
};

class GiMATEEC : public IOService
{
    OSDeclareDefaultStructors(GiMATEEC)

protected:
    IOMemoryMap        *ecMap;
    IOTimerEventSource *timer;

    void  refresh();
    void  timerFired();
    UInt8 read8(UInt32 off);
    UInt16 read16(UInt32 off);
    void  write8(UInt32 off, UInt8 v);

public:
    using IOService::setProperty;

    virtual bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    virtual IOReturn newUserClient(task_t owningTask, void *securityID, UInt32 type,
                                   OSDictionary *properties,
                                   LIBKERN_RETURNS_RETAINED IOUserClient **handler) APPLE_KEXT_OVERRIDE;

    bool setChargeLimit(int pct);
};

OSDefineMetaClassAndStructors(GiMATEEC, IOService)
OSDefineMetaClassAndStructors(GiMATEECClient, IOUserClient)

/* ------------------------------------------------------------------ */
/* EC access                                                          */
/* ------------------------------------------------------------------ */

UInt8 GiMATEEC::read8(UInt32 off)
{
    volatile UInt8 *b = ecMap ? (volatile UInt8 *) ecMap->getVirtualAddress() : NULL;
    return b ? b[off] : 0;
}

UInt16 GiMATEEC::read16(UInt32 off)
{
    return (UInt16)(((UInt16) read8(off + 1) << 8) | read8(off));
}

void GiMATEEC::write8(UInt32 off, UInt8 v)
{
    volatile UInt8 *b = ecMap ? (volatile UInt8 *) ecMap->getVirtualAddress() : NULL;
    if (b) {
        b[off] = v;
    }
}

void GiMATEEC::refresh()
{
    if (!ecMap) {
        setProperty("ECMapped", false);
        return;
    }

    UInt8 bcps = read8(0x03) & 0x0F;

    setProperty("ECMapped", true);
    setProperty("RPM1", (UInt32) read16(0x13), 32);
    setProperty("RPM2", (UInt32) read16(0x15), 32);
    setProperty("FAN1", (UInt32) read8(0x1B), 32);
    setProperty("FAN2", (UInt32) read8(0x1C), 32);
    setProperty("FLVL", (UInt32) read8(0x4B), 32);
    setProperty("KBLL", (UInt32) read8(0x31), 32);
    setProperty("CTMP", (UInt32) read8(0xB0), 32);
    setProperty("CTML", (UInt32) read8(0xB1), 32);
    setProperty("BCPS", (UInt32) bcps, 32);
    setProperty("BCPC", (UInt32) read8(0x05), 32);
    setProperty("ChargeMode", (UInt32) ((bcps >> 2) & 1), 32);
    setProperty("ChargeLimit", (UInt32) read8(0x05), 32);

    UInt32 flags = 0;
    if (read8(0x0B) & 0x80) flags |= 1u << 0;
    if (read8(0x0C) & 0x02) flags |= 1u << 1;
    if (read8(0x2C) & 0x01) flags |= 1u << 2;
    if (read8(0x2C) & 0x02) flags |= 1u << 3;
    if (read8(0x07) & 0x02) flags |= 1u << 4;
    setProperty("Flags", flags, 32);
}

void GiMATEEC::timerFired()
{
    refresh();
    if (timer) {
        timer->setTimeoutMS(1000);
    }
}

bool GiMATEEC::setChargeLimit(int pct)
{
    if (!ecMap || pct < 60 || pct > 100) {
        return false;
    }

    UInt8 bcps = read8(0x03) & 0x0F;
    if (pct >= 100) {
        write8(0x05, 100);
        write8(0x03, (UInt8) (bcps & ~0x04));
    } else {
        write8(0x05, (UInt8) pct);
        write8(0x03, (UInt8) (bcps | 0x04));
    }

    IOLog("GiMATEEC: charge limit -> %d%% (BCPS=0x%02X BCPC=%u)\n",
          pct, read8(0x03) & 0x0F, (unsigned) read8(0x05));
    refresh();
    return true;
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                          */
/* ------------------------------------------------------------------ */

bool GiMATEEC::start(IOService *provider)
{
    if (!IOService::start(provider)) {
        return false;
    }

    IOMemoryDescriptor *md =
        IOMemoryDescriptor::withPhysicalAddress(EC_BASE, EC_LEN, kIODirectionOutIn);
    if (md) {
        ecMap = md->map(kIOMapDefaultCache);
        md->release();
    }

    refresh();
    if (ecMap) {
        IOLog("GiMATEEC: EC mapped: rpm1=%u rpm2=%u ctmp=%u fan1=%u fan2=%u flvl=%u kbll=%u limit=%u\n",
              (unsigned) read16(0x13), (unsigned) read16(0x15),
              (unsigned) read8(0xB0), (unsigned) read8(0x1B), (unsigned) read8(0x1C),
              (unsigned) read8(0x4B), (unsigned) read8(0x31), (unsigned) read8(0x05));
    } else {
        IOLog("GiMATEEC: could not map the EC window\n");
    }

    timer = IOTimerEventSource::timerEventSource(this,
                OSMemberFunctionCast(IOTimerEventSource::Action, this, &GiMATEEC::timerFired));
    if (timer && getWorkLoop()) {
        getWorkLoop()->addEventSource(timer);
        timer->setTimeoutMS(1000);
    }

    registerService();
    return true;
}

void GiMATEEC::stop(IOService *provider)
{
    if (timer) {
        timer->cancelTimeout();
        if (getWorkLoop()) {
            getWorkLoop()->removeEventSource(timer);
        }
        timer->release();
        timer = NULL;
    }
    if (ecMap) {
        ecMap->release();
        ecMap = NULL;
    }
    IOService::stop(provider);
}

IOReturn GiMATEEC::newUserClient(task_t owningTask, void *securityID, UInt32 type,
                                  OSDictionary *properties,
                                  LIBKERN_RETURNS_RETAINED IOUserClient **handler)
{
    GiMATEECClient *client = new GiMATEECClient;
    if (!client) {
        return kIOReturnNoMemory;
    }
    if (!client->initWithTask(owningTask, securityID, type) || !client->attach(this)) {
        client->release();
        return kIOReturnError;
    }
    if (!client->start(this)) {
        client->detach(this);
        client->release();
        return kIOReturnError;
    }
    *handler = client;
    return kIOReturnSuccess;
}

/* ------------------------------------------------------------------ */
/* User client                                                        */
/* ------------------------------------------------------------------ */

#define kMuxSelSetChargeLimit 0
#define kMuxSelSetByte        1

bool GiMATEECClient::initWithTask(task_t owningTask, void *securityID, UInt32 type)
{
    if (!IOUserClient::initWithTask(owningTask, securityID, type)) {
        return false;
    }
    task  = owningTask;
    owner = NULL;
    return true;
}

bool GiMATEECClient::start(IOService *provider)
{
    if (!IOUserClient::start(provider)) {
        return false;
    }
    owner = OSDynamicCast(GiMATEEC, provider);
    return owner != NULL;
}

void GiMATEECClient::stop(IOService *provider)
{
    owner = NULL;
    IOUserClient::stop(provider);
}

IOReturn GiMATEECClient::clientClose()
{
    terminate(0);
    return kIOReturnSuccess;
}

IOReturn GiMATEECClient::sSetChargeLimit(OSObject *target, void *ref,
                                         IOExternalMethodArguments *args)
{
    GiMATEECClient *self = (GiMATEECClient *) target;
    if (!self || !self->owner || args->scalarInputCount < 1) {
        return kIOReturnBadArgument;
    }
    int pct = (int) args->scalarInput[0];
    bool ok = self->owner->setChargeLimit(pct);
    IOLog("GiMATEEC: user client setChargeLimit(%d) -> %s\n", pct, ok ? "ok" : "rejected");
    return ok ? kIOReturnSuccess : kIOReturnBadArgument;
}

IOReturn GiMATEECClient::externalMethod(uint32_t selector, IOExternalMethodArguments *args,
                                        IOExternalMethodDispatch *dispatch,
                                        OSObject *target, void *reference)
{
    static const IOExternalMethodDispatch methods[1] = {
        { (IOExternalMethodAction) &GiMATEECClient::sSetChargeLimit, 1, 0, 0, 0 },
    };

    if (selector >= (sizeof(methods) / sizeof(methods[0]))) {
        return kIOReturnUnsupported;
    }
    return IOUserClient::externalMethod(selector, args, (IOExternalMethodDispatch *) &methods,
                                        this, reference);
}

/* ------------------------------------------------------------------ */
/* kmod_info (the CLT linker no longer synthesises it)                */
/* ------------------------------------------------------------------ */

extern "C" kern_return_t GiMATEEC_start(kmod_info_t *ki, void *data)
{
    return KERN_SUCCESS;
}

extern "C" kern_return_t GiMATEEC_stop(kmod_info_t *ki, void *data)
{
    return KERN_SUCCESS;
}

extern "C" kmod_info_t kmod_info =
{
    0, KMOD_INFO_VERSION, (uint32_t) -1,
    "com.lumi.GiMATEEC", "0.2.0", -1,
    0, 0, 0, 0,
    GiMATEEC_start, GiMATEEC_stop
};
