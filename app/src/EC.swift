import Foundation
import IOKit

/// Reads the snapshot published by GiMATEEC.kext (IORegistry properties).
struct ECSnapshot {
    var mapped = false
    var rpm1 = 0, rpm2 = 0
    var fan1 = 0, fan2 = 0
    var flvl = 0, kbll = 0
    var ctmp = 0, ctml = 0
    var bcps = 0, bcpc = 0
    var flags = 0
}

enum EC {
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

        var input = UInt64(pct)
        let kr = IOConnectCallScalarMethod(conn, 0, &input, 1, nil, nil)
        return kr == KERN_SUCCESS
    }

    /// True when the internal panel is driven by the NVIDIA GPU (MUX = Discrete).
    static func panelOnDiscrete() -> Bool {
        let svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("NVRMDisplay"))
        guard svc != 0 else { return false }
        IOObjectRelease(svc)
        return true
    }
}
