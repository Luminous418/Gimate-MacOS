import Foundation

/// Thin wrapper around the user's `gigabyte-kbd` tool (HID LampArray).
/// The script is bundled in GiMATE.app/Contents/Resources; if it is not there,
/// the usual install locations are tried.
enum Keyboard {
    static var scriptPath: String? {
        if let p = Bundle.main.path(forResource: "gigabyte-kbd", ofType: nil),
           FileManager.default.fileExists(atPath: p) {
            return p
        }
        let candidates = [
            "/usr/local/bin/gigabyte-kbd",
            NSHomeDirectory() + "/.local/bin/gigabyte-kbd",
            NSHomeDirectory() + "/bin/gigabyte-kbd",
            NSHomeDirectory() + "/Proyectos/Gigabyte-Gaming-A16-macOS-KB/gigabyte-kbd",
        ]
        return candidates.first { FileManager.default.isExecutableFile(atPath: $0) }
    }

    static var available: Bool { scriptPath != nil }

    @discardableResult
    static func run(_ args: [String]) -> (code: Int32, output: String) {
        guard let path = scriptPath else { return (-1, "gigabyte-kbd not found") }
        let p = Process()
        p.executableURL = URL(fileURLWithPath: "/usr/bin/python3")
        p.arguments = [path] + args
        let pipe = Pipe()
        p.standardOutput = pipe
        p.standardError = pipe
        do {
            try p.run()
        } catch {
            return (-1, "\(error)")
        }
        let data = pipe.fileHandleForReading.readDataToEndOfFile()
        p.waitUntilExit()
        return (p.terminationStatus, (String(data: data, encoding: .utf8) ?? "").trimmingCharacters(in: .whitespacesAndNewlines))
    }

    @discardableResult
    static func setColor(_ r: UInt8, _ g: UInt8, _ b: UInt8, intensity: UInt8) -> (code: Int32, output: String) {
        let hex = String(format: "%02x%02x%02x", r, g, b)
        return run(["color", hex, "--intensity", "\(intensity)"])
    }

    @discardableResult
    static func off() -> (code: Int32, output: String) { run(["off"]) }

    @discardableResult
    static func autonomous(_ on: Bool) -> (code: Int32, output: String) {
        on ? run(["auto"]) : run(["color", String(format: "%02x%02x%02x", 0, 170, 255)])
    }
}
