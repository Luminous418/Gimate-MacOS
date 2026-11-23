import SwiftUI
import AppKit

@main
struct GiMATEApp: App {
    var body: some Scene {
        MenuBarExtra("GiMATE", systemImage: "fanblades.fill") {
            ContentView()
        }
        .menuBarExtraStyle(.window)
    }
}

struct ContentView: View {
    @State private var snap: ECSnapshot?
    @State private var discrete = false
    @State private var lamp: LampArray?
    @State private var hasLamp = false
    @State private var autonomous = false
    @State private var color = Color(red: 0.0, green: 0.66, blue: 1.0)
    @State private var timer: Timer?
    @State private var chargeLimit = 80

    private var tempText: String {
        guard let s = snap, s.mapped else { return "--" }
        return String(format: "%d.%02d °C", s.ctmp, s.ctml)
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("GIGABYTE A16").font(.headline)
                Spacer()
                Text(discrete ? "Discrete" : "Hybrid")
                    .font(.caption.weight(.semibold))
                    .padding(.horizontal, 7).padding(.vertical, 2)
                    .background(discrete ? Color.green.opacity(0.25) : Color.orange.opacity(0.25))
                    .clipShape(Capsule())
            }

            GroupBox("Sensors") {
                VStack(alignment: .leading, spacing: 4) {
                    row("CPU", tempText)
                    row("Fan 1", rpmFan(snap?.rpm1, snap?.fan1))
                    row("Fan 2", rpmFan(snap?.rpm2, snap?.fan2))
                    row("Level", snap.map { "\($0.flvl)" } ?? "--")
                    if snap?.mapped != true {
                        Text(snap == nil ? "GiMATEEC.kext not loaded" : "EC not mapped")
                            .font(.caption).foregroundStyle(.secondary)
                    }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }

            GroupBox("Battery") {
                HStack {
                    Text("Charge limit").foregroundStyle(.secondary)
                    Spacer()
                    Picker("", selection: $chargeLimit) {
                        Text("60 %").tag(60)
                        Text("80 %").tag(80)
                        Text("100 %").tag(100)
                    }
                    .labelsHidden()
                    .pickerStyle(.segmented)
                    .frame(width: 170)
                    .onChange(of: chargeLimit) { _, v in
                        _ = EC.setChargeLimit(v)
                    }
                }
                .font(.callout)
            }

            GroupBox("Keyboard") {
                VStack(alignment: .leading, spacing: 8) {
                    if hasLamp {
                        ColorPicker("Colour", selection: $color, supportsOpacity: false)
                        HStack {
                            Button("Apply") { applyColor() }
                            Button("Off") { lamp?.off() }
                            Spacer()
                            Toggle("Auto", isOn: $autonomous)
                                .onChange(of: autonomous) { _, v in lamp?.setAutonomous(v) }
                        }
                        .controlSize(.small)
                    } else {
                        Text("LampArray not found")
                            .font(.caption).foregroundStyle(.secondary)
                    }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }

            HStack {
                Text(snap == nil ? "kext: –" : "kext: ok")
                    .font(.caption2).foregroundStyle(.secondary)
                Spacer()
                Button("Quit") { NSApplication.shared.terminate(nil) }
                    .controlSize(.small)
            }
        }
        .padding(12)
        .frame(width: 300)
        .onAppear(perform: start)
        .onDisappear { timer?.invalidate() }
    }

    private func row(_ title: String, _ value: String) -> some View {
        HStack {
            Text(title).foregroundStyle(.secondary)
            Spacer()
            Text(value).monospacedDigit()
        }
        .font(.callout)
    }

    private func rpmFan(_ rpm: Int?, _ duty: Int?) -> String {
        guard let rpm, rpm >= 0 else { return "--" }
        if let duty { return "\(rpm) rpm  (\(duty) %)" }
        return "\(rpm) rpm"
    }

    private func start() {
        lamp = LampArray()
        hasLamp = lamp != nil
        refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in refresh() }
    }

    private func refresh() {
        snap = EC.read()
        discrete = EC.panelOnDiscrete()
        if let s = snap, s.mapped, chargeLimit != s.bcpc {
            chargeLimit = s.bcpc
        }
    }

    private func applyColor() {
        let ns = NSColor(color).usingColorSpace(.sRGB) ?? .cyan
        lamp?.setColor(red: UInt8(ns.redComponent * 255),
                       green: UInt8(ns.greenComponent * 255),
                       blue: UInt8(ns.blueComponent * 255))
    }
}
