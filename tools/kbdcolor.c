// kbdcolor - set the GIGABYTE A16 keyboard colour (HID LampArray).
// Usage: kbdcolor RRGGBB [intensity 0-255]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <CoreFoundation/CoreFoundation.h>

#define VID 0x0414
#define USAGE_PAGE 0x59
#define USAGE 1

static int getFeature(IOHIDDeviceRef dev, uint8_t id, uint8_t *buf, CFIndex len)
{
    CFIndex n = len;
    return IOHIDDeviceGetReport(dev, kIOHIDReportTypeFeature, id, buf, &n);
}

static int setFeature(IOHIDDeviceRef dev, const uint8_t *buf, CFIndex len)
{
    // buf[0] is the report ID, IOKit wants it separately and the payload after it
    return IOHIDDeviceSetReport(dev, kIOHIDReportTypeFeature, buf[0], buf + 1, len - 1);
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "uso: kbdcolor RRGGBB [intensidad]\n"); return 1; }
    unsigned r, g, b;
    if (sscanf(argv[1], "%2x%2x%2x", &r, &g, &b) != 3) {
        fprintf(stderr, "color no válido (RRGGBB)\n"); return 1;
    }
    unsigned intensity = (argc > 2) ? (unsigned) atoi(argv[2]) : 255;

    IOHIDManagerRef mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    CFMutableDictionaryRef match = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    int vid = VID, up = USAGE_PAGE, u = USAGE;
    CFNumberRef nv = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vid);
    CFNumberRef np = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &up);
    CFNumberRef nu = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &u);
    CFDictionarySetValue(match, CFSTR(kIOHIDVendorIDKey), nv);
    CFDictionarySetValue(match, CFSTR("PrimaryUsagePage"), np);
    CFDictionarySetValue(match, CFSTR("PrimaryUsage"), nu);
    IOHIDManagerSetDeviceMatching(mgr, match);

    IOReturn rc = IOHIDManagerOpen(mgr, kIOHIDOptionsTypeNone);
    if (rc != kIOReturnSuccess) { fprintf(stderr, "IOHIDManagerOpen: 0x%x\n", rc); return 2; }

    CFSetRef devs = IOHIDManagerCopyDevices(mgr);
    if (!devs || CFSetGetCount(devs) == 0) { fprintf(stderr, "no encuentro el LampArray\n"); return 3; }
    const void *values[8] = {0};
    CFSetGetValues(devs, values);
    IOHIDDeviceRef dev = (IOHIDDeviceRef) values[0];

    rc = IOHIDDeviceOpen(dev, kIOHIDOptionsTypeNone);
    if (rc != kIOReturnSuccess) { fprintf(stderr, "IOHIDDeviceOpen: 0x%x\n", rc); return 4; }

    uint8_t attr[32] = {0};
    int count = 0;
    if (getFeature(dev, 0x01, attr, 23) == kIOReturnSuccess) {
        count = attr[1] | (attr[2] << 8);   // payload after the ID byte
        printf("lamps: %d\n", count);
    } else {
        printf("no pude leer LampArrayAttributes, uso 1 lamp\n");
        count = 1;
    }

    uint8_t autoOff[2] = { 0x06, 0 };
    setFeature(dev, autoOff, 2);

    uint8_t rep[10] = { 0x05, 0x01,
                       0x00, 0x00,
                       (uint8_t)((count - 1) & 0xFF), (uint8_t)((count - 1) >> 8),
                       (uint8_t) r, (uint8_t) g, (uint8_t) b, (uint8_t) intensity };
    rc = setFeature(dev, rep, 10);
    printf("set report 0x05 -> 0x%x  (color #%02x%02x%02x, intensidad %u)\n", rc, r, g, b, intensity);

    IOHIDDeviceClose(dev, kIOHIDOptionsTypeNone);
    return rc == kIOReturnSuccess ? 0 : 5;
}
