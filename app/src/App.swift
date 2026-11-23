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

/// Opens the shared NSColorPanel from an accessory (menu bar) app.
final class ColorPanelBridge: NSObject {
    static let shared = ColorPanelBridge()
    private var handler: ((NSColor) -> Void)?

    func show(from color: NSColor, handler: @escaping (NSColor) -> Void) {
        self.handler = handler
        let panel = NSColorPanel.shared
        panel.showsAlpha = false
        panel.isContinuous = true
        panel.setTarget(self)
        panel.setAction(#selector(changed(_:)))
        panel.color = color
        NSApp.activate(ignoringOtherApps: true)
        panel.makeKeyAndOrderFront(nil)
    }

    @objc private func changed(_ sender: NSColorPanel) {
        handler?(sender.color)
    }
}

struct ContentView: View {
    @State private var snap: ECSnapshot?
    @State private var discrete = false
    @State private var hasLamp = Keyboard.available
    @State private var kbdStatus = ""
    @State private var autonomous = false
    @State private var brightness: Double = 1.0
    @State private var lastColor: (UInt8, UInt8, UInt8) = (0, 170, 255)
    @State private var timer: Timer?
    @State private var chargeLimit = 80
    @State private var fanMode = 3

    private let presets: [(String, (UInt8, UInt8, UInt8))] = [
        ("Blanco",  (255, 255, 255)),
        ("Rojo",    (255, 0, 0)),
        ("Naranja", (255, 120, 0)),
        ("Amarillo",(255, 220, 0)),
        ("Verde",   (0, 255, 60)),
        ("Cian",    (0, 170, 255)),
        ("Azul",    (0, 40, 255)),
        ("Magenta", (200, 0, 255)),
    ]

    private var tempText: String {
        guard let s = snap, s.mapped else { return "--" }
        return "\(s.ctmp) °C"
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("Gigabyte Gaming A16").font(.headline)
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
                    if snap?.mapped != true {
                        Text(snap == nil ? "GiMATEEC.kext not loaded" : "EC not mapped")
                            .font(.caption).foregroundStyle(.secondary)
                    }
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }

            GroupBox("Fan") {
                Picker("", selection: $fanMode) {
                    Text("Normal").tag(0)
                    Text("Silent").tag(1)
                    Text("Gaming").tag(2)
                    Text("Auto").tag(3)
                }
                .labelsHidden()
                .pickerStyle(.segmented)
                .onChange(of: fanMode) { _, v in _ = EC.setFanMode(v) }
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
                    .onChange(of: chargeLimit) { _, v in _ = EC.setChargeLimit(v) }
                }
                .font(.callout)
            }

            GroupBox("Keyboard") {
                VStack(alignment: .leading, spacing: 8) {
                    if hasLamp {
                        HStack(spacing: 6) {
                            ForEach(presets, id: \.0) { name, rgb in
                                Circle()
                                    .fill(Color(red: Double(rgb.0) / 255,
                                                green: Double(rgb.1) / 255,
                                                blue: Double(rgb.2) / 255))
                                    .frame(width: 22, height: 22)
                                    .overlay(Circle().stroke(Color.primary.opacity(0.25)))
                                    .help(name)
                                    .onTapGesture { apply(rgb) }
                            }
                            Spacer()
                            Button {
                                openColorPanel()
                            } label: {
                                Image(systemName: "eyedropper")
                            }
                            .buttonStyle(.borderless)
                            .help("Elegir otro color")
                            Button {
                                let r = Keyboard.off()
                                kbdStatus = r.code == 0 ? "" : r.output
                            } label: {
                                Image(systemName: "power")
                            }
                            .buttonStyle(.borderless)
                            .help("Apagar")
                        }

                        HStack {
                            Image(systemName: "sun.min")
                            Slider(value: $brightness, in: 0...1) { editing in
                                if !editing { apply(lastColor) }
                            }
                            Image(systemName: "sun.max")
                        }
                        .font(.caption)

                        if !kbdStatus.isEmpty {
                            Text(kbdStatus).font(.caption2).foregroundStyle(.red).lineLimit(2)
                        }

                        Toggle("Modo autónomo (Fn+Space)", isOn: $autonomous)
                            .font(.caption)
                            .onChange(of: autonomous) { _, v in
                                let r = Keyboard.autonomous(v)
                                kbdStatus = r.code == 0 ? "" : r.output
                            }
                    } else {
                        Text("gigabyte-kbd not found")
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
        .frame(width: 320)
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
        hasLamp = Keyboard.available
        refresh()
        timer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { _ in refresh() }
    }

    private func refresh() {
        snap = EC.read()
        discrete = EC.panelOnDiscrete()
        if let s = snap, s.mapped {
            if chargeLimit != s.bcpc { chargeLimit = s.bcpc }
            if fanMode != s.fanMode { fanMode = s.fanMode }
        }
    }

    private func openColorPanel() {
        let base = NSColor(srgbRed: CGFloat(lastColor.0) / 255,
                           green: CGFloat(lastColor.1) / 255,
                           blue: CGFloat(lastColor.2) / 255,
                           alpha: 1)
        ColorPanelBridge.shared.show(from: base) { ns in
            let c = ns.usingColorSpace(.sRGB) ?? .cyan
            apply((UInt8(max(0, min(255, c.redComponent * 255))),
                   UInt8(max(0, min(255, c.greenComponent * 255))),
                   UInt8(max(0, min(255, c.blueComponent * 255)))))
        }
    }

    private func apply(_ rgb: (UInt8, UInt8, UInt8)) {
        lastColor = rgb
        let i = UInt8(max(0, min(255, brightness * 255)))
        let r = Keyboard.setColor(rgb.0, rgb.1, rgb.2, intensity: i)
        kbdStatus = r.code == 0 ? "" : r.output
    }
}
