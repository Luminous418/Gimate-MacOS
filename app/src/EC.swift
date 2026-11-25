import Foundation
import IOKit
import IOKit.ps

/// Battery information published by macOS (AppleSmartBattery). Read-only.
struct BatteryInfo {
    var cycles = 0
    var design = 0
    var maxCapacity = 0
    /// Percentage exactly as macOS shows it (smoothed by AppleSmartBatteryManager).
    var percent = 0
    var charging = false

    var healthPercent: Int {
        guard design > 0 else { return 0 }
        return Int((Double(maxCapacity) / Double(design) * 100).rounded())
    }
    var chargePercent: Int { percent }
}

/// Reads the snapshot published by GiMATEEC.kext (IORegistry properties).
struct ECSnapshot {
    var mapped = false
    var rpm1 = 0, rpm2 = 0
    var fan1 = 0, fan2 = 0
    var flvl = 0, kbll = 0
    var ctmp = 0, ctml = 0
    var bcps = 0, bcpc = 0
    var fanMode = 0
    var muxState = -1
    var flags = 0
}

enum EC {
    static func battery() -> BatteryInfo? {
        let svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("AppleSmartBattery"))
        guard svc != 0 else { return nil }
        defer { IOObjectRelease(svc) }

        var props: Unmanaged<CFMutableDictionary>?
        guard IORegistryEntryCreateCFProperties(svc, &props, kCFAllocatorDefault, 0) == KERN_SUCCESS,
              let dict = props?.takeRetainedValue() as? [String: Any] else { return nil }

        func i(_ k: String) -> Int { (dict[k] as? NSNumber)?.intValue ?? 0 }
        var b = BatteryInfo()
        b.cycles = i("CycleCount")
        b.design = i("DesignCapacity")
        b.maxCapacity = i("MaxCapacity")
        if b.maxCapacity == 0 { b.maxCapacity = i("AppleRawMaxCapacity") }

        // Percentage and charging state from the power-sources API, which is
        // what the menu bar and pmset show (the fuel-gauge ratio can differ
        // by a few points).
        if let blob = IOPSCopyPowerSourcesInfo()?.takeRetainedValue(),
           let list = IOPSCopyPowerSourcesList(blob)?.takeRetainedValue() as? [CFTypeRef] {
            for ps in list {
                guard let d = IOPSGetPowerSourceDescription(blob, ps)?.takeUnretainedValue() as? [String: Any],
                      (d[kIOPSTypeKey] as? String) == kIOPSInternalBatteryType else { continue }
                b.percent = d[kIOPSCurrentCapacityKey] as? Int ?? 0
                b.charging = d[kIOPSIsChargingKey] as? Bool ?? false
                break
            }
        }
        return b
    }

    static func read() -> ECSnapshot? {
        let svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("GiMATEEC"))
        guard svc != 0 else { return nil }
        defer { IOObjectRelease(svc) }

        var props: Unmanaged<CFMutableDictionary>?
        guard IORegistryEntryCreateCFProperties(svc, &props, kCFAllocatorDefault, 0) == KERN_SUCCESS,
              let dict = props?.takeRetainedValue() as? [String: Any] else { return nil }

        func i(_ k: String) -> Int { (dict[k] as? NSNumber)?.intValue ?? 0 }

        var s = ECSnapshot()
        s.mapped = i("ECMapped") != 0
        s.rpm1 = i("RPM1"); s.rpm2 = i("RPM2")
        s.fan1 = i("FAN1"); s.fan2 = i("FAN2")
        s.flvl = i("FLVL"); s.kbll = i("KBLL")
        s.ctmp = i("CTMP"); s.ctml = i("CTML")
        s.bcps = i("BCPS"); s.bcpc = i("BCPC")
        s.fanMode = i("FanMode")
        s.muxState = (dict["MuxState"] != nil) ? i("MuxState") : -1
        s.flags = i("Flags")
        return s
    }

    /// Writes the charge limit (60..100) through the kext user client.
    static func setChargeLimit(_ pct: Int) -> Bool {
        let svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("GiMATEEC"))
        guard svc != 0 else { return false }
        defer { IOObjectRelease(svc) }

        var conn: io_connect_t = 0
        guard IOServiceOpen(svc, mach_task_self_, 0, &conn) == KERN_SUCCESS else { return false }
        defer { IOServiceClose(conn) }

        var input: [UInt64] = [0, UInt64(pct)]
        let kr = IOConnectCallScalarMethod(conn, 0, &input, 2, nil, nil)
        return kr == KERN_SUCCESS
    }

    /// Writes the fan mode through the kext user client (1 silent, 2 gaming, 3 auto).
    static func setFanMode(_ mode: Int) -> Bool {
        let svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("GiMATEEC"))
        guard svc != 0 else { return false }
        defer { IOObjectRelease(svc) }

        var conn: io_connect_t = 0
        guard IOServiceOpen(svc, mach_task_self_, 0, &conn) == KERN_SUCCESS else { return false }
        defer { IOServiceClose(conn) }

        var input: [UInt64] = [1, UInt64(mode)]
        return IOConnectCallScalarMethod(conn, 0, &input, 2, nil, nil) == KERN_SUCCESS
    }

    /// True when the panel is on the NVIDIA GPU (MUX = Discrete).
    ///
    /// Preferred source is the EC (CMOS 0xD0): 0 = MSHybrid, 1 = Discrete.
    /// Without GiMATEEC, fall back to which driver owns the internal panel:
    /// with the Intel framebuffer present the panel is on the iGPU.
    static func panelOnDiscrete() -> Bool {
        if let snaps = read(), snaps.muxState >= 0 {
            return snaps.muxState == 1
        }

        let intel = IOServiceGetMatchingService(kIOMainPortDefault,
                                                IOServiceMatching("AppleIntelFramebuffer"))
        if intel != 0 {
            IOObjectRelease(intel)
            return false
        }

        let nv = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("NVRMDisplay"))
        guard nv != 0 else { return false }
        IOObjectRelease(nv)
        return true
    }
}
