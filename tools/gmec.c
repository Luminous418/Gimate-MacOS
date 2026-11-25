// gmec - read GiMATEEC values from the IORegistry.
// Usage: gmec [-w]      (-w: keep printing once per second)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <IOKit/IOKitLib.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>

static int printSnapshot(io_service_t svc)
{
    CFMutableDictionaryRef props = NULL;
    if (IORegistryEntryCreateCFProperties(svc, &props, kCFAllocatorDefault, 0) != KERN_SUCCESS || !props) {
        return 1;
    }

    CFNumberRef n;
#define GETI(k) ({ int v = -1; if ((n = CFDictionaryGetValue(props, CFSTR(k)))) CFNumberGetValue(n, kCFNumberIntType, &v); v; })
    int mapped = GETI("ECMapped");
    int rpm1 = GETI("RPM1"), rpm2 = GETI("RPM2");
    int fan1 = GETI("FAN1"), fan2 = GETI("FAN2");
    int flvl = GETI("FLVL"), kbll = GETI("KBLL");
    int ctmp = GETI("CTMP"), ctml = GETI("CTML");
    int bcps = GETI("BCPS"), bcpc = GETI("BCPC");
    int fanmode = GETI("FanMode");
    int flags = GETI("Flags");
    int mux = GETI("MuxState"), muxraw = GETI("MuxRaw"), leds = GETI("LedState");
    int lastOp = GETI("LastOp"), lastVal = GETI("LastValue"), lastRes = GETI("LastResult");
#undef GETI

    printf("Leds %d | Mux %s (raw 0x%02X) | FanMode %d | last=%d/%d/%d | ",
           leds, mux == 1 ? "Discrete" : (mux == 0 ? "MSHybrid" : "?"), muxraw, fanmode, lastOp, lastVal, lastRes);
    printf("EC %s | CPU %d.%02d C | fan1 %4d rpm (%d%%) | fan2 %4d rpm (%d%%) | FLVL %d | KBLL %d | BCPS %d BCPC %d | flags 0x%x\n",
           mapped ? "ok  " : "FAIL", ctmp, ctml, rpm1, fan1, rpm2, fan2, flvl, kbll, bcps, bcpc, flags);

    CFDataRef raw = CFDictionaryGetValue(props, CFSTR("Raw"));
    if (raw && CFDataGetLength(raw) >= 256) {
        const UInt8 *p = CFDataGetBytePtr(raw);
        for (int row = 0; row < 16; row++) {
            printf("  %02X:", row * 16);
            for (int col = 0; col < 16; col++) printf(" %02X", p[row * 16 + col]);
            printf("\n");
        }
    }

    CFRelease(props);
    return 0;
}

int main(int argc, char **argv)
{
    int watch = (argc > 1 && strcmp(argv[1], "-w") == 0);
    int setCharge = -1;
    if (argc > 2 && strcmp(argv[1], "charge") == 0) {
        setCharge = atoi(argv[2]);
    }
    int setLeds = -1;
    if (argc > 2 && strcmp(argv[1], "led") == 0) {
        setLeds = atoi(argv[2]);
    }
    int setFan = -1;
    if (argc > 2 && strcmp(argv[1], "fan") == 0) {
        if (strcmp(argv[2], "normal") == 0) setFan = 0;
        else if (strcmp(argv[2], "silent") == 0) setFan = 1;
        else if (strcmp(argv[2], "gaming") == 0) setFan = 2;
        else if (strcmp(argv[2], "auto") == 0) setFan = 3;
        else setFan = atoi(argv[2]);
    }

    io_service_t svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("GiMATEEC"));
    if (!svc) {
        fprintf(stderr, "GiMATEEC not loaded (kext missing?)\n");
        return 2;
    }

    if (setCharge >= 0 || setFan >= 0 || setLeds >= 0) {
        io_connect_t conn = 0;
        kern_return_t kr = IOServiceOpen(svc, mach_task_self(), 0, &conn);
        if (kr != KERN_SUCCESS) {
            fprintf(stderr, "IOServiceOpen failed: 0x%x\n", kr);
            IOObjectRelease(svc);
            return 3;
        }
        uint64_t in[2];
        if (setLeds >= 0) { in[0] = 2; in[1] = (uint64_t) setLeds; }
        else { in[0] = (setCharge >= 0) ? 0 : 1; in[1] = (setCharge >= 0) ? (uint64_t) setCharge : (uint64_t) setFan; }
        kr = IOConnectCallScalarMethod(conn, 0, in, 2, NULL, NULL);
        IOServiceClose(conn);
        if (kr != KERN_SUCCESS) {
            fprintf(stderr, "set failed: 0x%x\n", kr);
            IOObjectRelease(svc);
            return 3;
        }
        usleep(400000);
    }

    do {
        if (printSnapshot(svc) != 0) {
            fprintf(stderr, "cannot read properties\n");
            break;
        }
        if (watch) sleep(1);
    } while (watch);

    IOObjectRelease(svc);
    return 0;
}
