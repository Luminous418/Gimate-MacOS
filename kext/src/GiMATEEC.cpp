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

    static IOReturn sSet(OSObject *target, void *ref, IOExternalMethodArguments *args);
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
    bool setFanMode(int mode);
};

OSDefineMetaClassAndStructors(GiMATEEC, IOService)
OSDefineMetaClassAndStructors(GiMATEECClient, IOUserClient)

/* ------------------------------------------------------------------ */
/* EC access                                                          */
/* ------------------------------------------------------------------ */

static inline UInt8 inbPort(UInt16 port)
{
    UInt8 v;
    __asm__ volatile ("inb %1, %0" : "=a" (v) : "Nd" (port));
    return v;
}

static inline void outbPort(UInt16 port, UInt8 v)
{
    __asm__ volatile ("outb %0, %1" :: "a" (v), "Nd" (port));
}

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

    /* FanMode: 0 normal, 1 silent, 2 gaming, 3 auto (from the EC bits) */
    int fanMode = 0;
    if (read8(0x0C) & 0x02) {
        fanMode = 3;                       /* GFAN = auto */
    } else if (read8(0x2C) & 0x02) {
        fanMode = 2;                       /* FANB = gaming */
    } else if (read8(0x2C) & 0x01) {
        fanMode = 1;                       /* CRAF = silent */
    }
    setProperty("FanMode", (UInt32) fanMode, 32);

    /* Display MUX: extended CMOS index 0xD0 (ports 0x72/0x73).
     * 0 = MSHybrid (panel on the iGPU), 1 = Discrete (panel on the dGPU). */
    outbPort(0x72, 0xD0);
    UInt8 muxRaw = inbPort(0x73);
    setProperty("MuxRaw", (UInt32) muxRaw, 32);
    setProperty("MuxState", (UInt32) (muxRaw & 0x01), 32);
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

bool GiMATEEC::setFanMode(int mode)
{
    if (!ecMap) {
        return false;
    }

    UInt8 tfan = read8(0x0B) & 0x7F;
    UInt8 gfan = read8(0x0C) & 0xFD;
    UInt8 ramp = read8(0x2C) & 0xFC;

    switch (mode) {
    case 0:                     /* normal: no mode bit set */
        write8(0x2C, ramp);
        write8(0x0C, gfan);
        write8(0x0B, tfan);
        break;
    case 1:                     /* silent */
        write8(0x0B, tfan);
        write8(0x2C, (UInt8) (ramp | 0x01));
        write8(0x0C, gfan);
        break;
    case 2:                     /* gaming */
        write8(0x0B, tfan);
        write8(0x2C, (UInt8) (ramp | 0x02));
        write8(0x0C, gfan);
        break;
    case 3:                     /* auto */
        write8(0x2C, ramp);
        write8(0x0B, tfan);
        write8(0x0C, (UInt8) (gfan | 0x02));
        break;
    default:
        return false;
    }

    IOLog("GiMATEEC: fan mode -> %d (0B=0x%02X 0C=0x%02X 2C=0x%02X)\n",
          mode, read8(0x0B), read8(0x0C), read8(0x2C));
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
#define kMuxSelSetFanMode     1

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

IOReturn GiMATEECClient::sSet(OSObject *target, void *ref,
                                IOExternalMethodArguments *args)
{
    GiMATEECClient *self = (GiMATEECClient *) target;
    if (!self || !self->owner || args->scalarInputCount < 2) {
        return kIOReturnBadArgument;
    }

    int op    = (int) args->scalarInput[0];
    int value = (int) args->scalarInput[1];

    bool ok = false;
    if (op == 0) {
        ok = self->owner->setChargeLimit(value);
    } else if (op == 1) {
        ok = self->owner->setFanMode(value);
    }

    self->owner->setProperty("LastOp", (UInt32) op, 32);
    self->owner->setProperty("LastValue", (UInt32) value, 32);
    self->owner->setProperty("LastResult", (UInt32) (ok ? 1 : 0), 32);

    return ok ? kIOReturnSuccess : kIOReturnBadArgument;
}

IOReturn GiMATEECClient::externalMethod(uint32_t selector, IOExternalMethodArguments *args,
                                        IOExternalMethodDispatch *dispatch,
                                        OSObject *target, void *reference)
{
    static const IOExternalMethodDispatch methods[1] = {
        { (IOExternalMethodAction) &GiMATEECClient::sSet, 2, 0, 0, 0 },
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
