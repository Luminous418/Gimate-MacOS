import Foundation
import IOKit
import IOKit.hid

/// Minimal HID LampArray client for the GIGABYTE A16 keyboard.
///
/// Report map (feature reports, sizes include the report ID byte):
///   0x01 LampArrayAttributes  1 + 2 + 5*4 = 23
///   0x05 LampRangeUpdate      1 + 1 + 4 + 4 = 10
///   0x06 LampArrayControl     1 + 1 = 2
final class LampArray {
    private var manager: IOHIDManager?
    private var device: IOHIDDevice?
    private(set) var lampCount: Int = 0

    static let vendorID = 0x0414
    static let usagePage = 0x59
    static let usage = 0x01

    /// Returns nil when the keyboard's LampArray interface is not present.
    init?() {
        let mgr = IOHIDManagerCreate(kCFAllocatorDefault, IOOptionBits(kIOHIDOptionsTypeNone))
        let match: [String: Any] = [
            kIOHIDVendorIDKey: Self.vendorID,
            "PrimaryUsagePage": Self.usagePage,
            "PrimaryUsage": Self.usage,
        ]
        IOHIDManagerSetDeviceMatching(mgr, match as CFDictionary)
        guard IOHIDManagerOpen(mgr, IOOptionBits(kIOHIDOptionsTypeNone)) == kIOReturnSuccess,
              let devices = IOHIDManagerCopyDevices(mgr) as? Set<IOHIDDevice>,
              let dev = devices.first else {
            return nil
        }
        guard IOHIDDeviceOpen(dev, IOOptionBits(kIOHIDOptionsTypeNone)) == kIOReturnSuccess else {
            return nil
        }
        manager = mgr
        device = dev
        lampCount = readLampCount() ?? 0
    }

    deinit {
        if let d = device { IOHIDDeviceClose(d, IOOptionBits(kIOHIDOptionsTypeNone)) }
    }

    private func getFeature(_ reportID: UInt8, size: Int) -> [UInt8]? {
        guard let dev = device else { return nil }
        var buf = [UInt8](repeating: 0, count: size)
        var len = size
        let rc = IOHIDDeviceGetReport(dev, kIOHIDReportTypeFeature, CFIndex(reportID), &buf, &len)
        guard rc == kIOReturnSuccess else { return nil }
        return Array(buf.dropFirst())
    }

    private func setFeature(_ bytes: [UInt8]) {
        guard let dev = device, let first = bytes.first else { return }
        var payload = Array(bytes.dropFirst())
        _ = IOHIDDeviceSetReport(dev, kIOHIDReportTypeFeature, CFIndex(first), &payload, payload.count)
    }

    private func readLampCount() -> Int? {
        guard let p = getFeature(0x01, size: 23), p.count >= 3 else { return nil }
        return Int(p[1]) | (Int(p[2]) << 8)
    }

    /// Host mode on/off (firmware drives the LEDs when enabled).
    func setAutonomous(_ on: Bool) {
        setFeature([0x06, on ? 1 : 0])
    }

    func setColor(red: UInt8, green: UInt8, blue: UInt8, intensity: UInt8 = 255) {
        let count = max(lampCount, 1)
        setAutonomous(false)
        let end = count - 1
        setFeature([
            0x05, 0x01,
            UInt8(0 & 0xFF), UInt8(0 >> 8),
            UInt8(end & 0xFF), UInt8(end >> 8),
            red, green, blue, intensity,
        ])
    }

    func off() { setColor(red: 0, green: 0, blue: 0, intensity: 0) }
}
