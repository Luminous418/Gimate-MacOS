import Foundation
import IOKit
import IOKit.hid

// Reproduce what the app does, printing every return code.

let vid = 0x0414, usagePage = 0x59, usage = 0x01

let mgr = IOHIDManagerCreate(kCFAllocatorDefault, IOOptionBits(kIOHIDOptionsTypeNone))
let match: [String: Any] = [
    kIOHIDVendorIDKey: vid,
    "PrimaryUsagePage": usagePage,
    "PrimaryUsage": usage,
]
IOHIDManagerSetDeviceMatching(mgr, match as CFDictionary)
var rc = IOHIDManagerOpen(mgr, IOOptionBits(kIOHIDOptionsTypeNone))
print("IOHIDManagerOpen: 0x\(String(rc, radix: 16))")
guard rc == kIOReturnSuccess else { exit(2) }

guard let devices = IOHIDManagerCopyDevices(mgr) as? Set<IOHIDDevice> else {
    print("copy devices failed"); exit(3)
}
print("devices: \(devices.count)")
guard let dev = devices.first else { print("no device"); exit(3) }

rc = IOHIDDeviceOpen(dev, IOOptionBits(kIOHIDOptionsTypeNone))
print("IOHIDDeviceOpen: 0x\(String(rc, radix: 16))")
guard rc == kIOReturnSuccess else { exit(4) }

// attributes
var attr = [UInt8](repeating: 0, count: 23)
var attrLen = 23
rc = IOHIDDeviceGetReport(dev, kIOHIDReportTypeFeature, 0x01, &attr, &attrLen)
print("GetReport 0x01: 0x\(String(rc, radix: 16)) len=\(attrLen) first=\(attr.prefix(6).map { String(format: "%02x", $0) }.joined(separator: " "))")
let count = Int(attr[1]) | (Int(attr[2]) << 8)
print("lampCount=\(count)")

// autonomous off
var autoRep: [UInt8] = [0x00]
rc = IOHIDDeviceSetReport(dev, kIOHIDReportTypeFeature, 0x06, &autoRep, 1)
print("SetReport 0x06: 0x\(String(rc, radix: 16))")

// range
let end = max(count, 1) - 1
var rep: [UInt8] = [0x01, UInt8(0), UInt8(0), UInt8(end & 0xFF), UInt8(end >> 8), 255, 0, 255, 255]
rc = IOHIDDeviceSetReport(dev, kIOHIDReportTypeFeature, 0x05, &rep, rep.count)
print("SetReport 0x05: 0x\(String(rc, radix: 16)) payload=\(rep.map { String(format: "%02x", $0) }.joined(separator: " "))")

IOHIDDeviceClose(dev, IOOptionBits(kIOHIDOptionsTypeNone))
