# GiMATE for macOS

A native macOS replacement for **GiMATE / Gigabyte Control Center** on the
**GIGABYTE GAMING A16 (GA6H)**: keyboard RGB, fan profiles, battery charge limit,
display MUX and sensors — without Windows, Wine or a VM.

> 🚧 **Work in progress.** Nothing here is a finished product yet. See *Status*.

---

## Why

On Windows, GiMATE is the only thing that lights the keyboard backlight at boot, applies
the fan/power profile and switches the display MUX. Under macOS all of that is missing:
the EC keeps its defaults and nothing talks to it.

This project reverse-engineers the same interfaces GiMATE uses and exposes them as a normal
macOS app.

## What we already know

From disassembling `GiMATE.dll` and `Gbt.GpuPowerGear.Hardware.dll` (the .NET app), and from
the ACPI tables of this laptop:

| Interface | Detail |
|---|---|
| **WMI / ACPI** | device `AMW0` (`PNP0C14`), classes `GB_WMIACPI_Get/Set/Data/Event`. Methods are dispatched by `WMBC` (read) / `WMBD` (write) with a `WmiMethodId` in `Arg1` |
| **Display MUX** | `WmiMethodId 0xE6` → extended CMOS index `0xD0` (ports `0x72/0x73`): `0` = MSHybrid, `1` = Discrete. Consumed at POST. Already solved in [muxfix](https://github.com/Luminous418/muxfix) |
| **EC memory window** | `OperationRegion (PECM, SystemMemory, 0xFE0B0800, 0x1000)`. Holds fan, LED, battery and power-mode fields (`GFAN`, `FAN1/2`, `TFAN`, `FLVL`, `CRAF`, `ADJF`, `XFNW`, `PFMM`, `BCPS/BCPC`, `BLED/PLED`, …) |
| **Keyboard RGB** | standard **HID LampArray** (Usage Page `0x59`) on USB `0414:8105` — pure userspace, already solved in [gigabyte-kbd](https://github.com/Luminous418/Gigabyte-Gaming-A16-macOS-KB) |

## Architecture (planned)

```
GiMATE.app        SwiftUI menu-bar app     ← UI, no privileges
      │ XPC
gmecd             root daemon              ← applies settings at boot, owns the kext
      │
GiMATEEC.kext     maps the EC window       ← only if read/write of the EC is needed
      └── plus IOKit HID for the RGB (no kext)
```

## Status

| Phase | Scope | State |
|---|---|---|
| **1. MVP** | Menu-bar app: keyboard RGB, MUX status, EC sensors (`RPM1/RPM2`, `CTMP`) | ✅ working |
| 2. Charge limit | 60 / 80 / 100 % (BCPC + BCPS bit 2) | ✅ register write verified |
| 3. Fan profile | Silent / Gaming / Auto (`CRAF`, `FANB`, `GFAN`) | ✅ implemented, pending on-hardware test |
| 4. LEDs / extras | chassis LEDs, keyboard brightness | pending |
| 5. Manual fan curve | — | **not planned: too risky** |

## Method

Every hardware feature is identified the same way, and it is the method that worked for the
MUX: on Windows, dump the EC window (RWEverything) with GiMATE closed, change **one** setting
in GiMATE, dump again and diff. The byte that changes is the field.

## Safety

- No firmware/BIOS flashing, ever.
- No raw PWM or voltage writes.
- Read-only first; writes only for fields confirmed against the vendor app.
- Everything reversible: the EC resets on power loss, and the daemon re-applies on boot.

## Repository layout

```
kext/      GiMATEEC.kext: read-only mapping of the EC window (0xFE0B0800)
tools/     gmec (CLI), deployment scripts
app/       GiMATE.app: SwiftUI menu-bar app (sensors, fan mode, charge limit,
           RGB via the bundled gigabyte-kbd script, MUX state)
audit/     macOS hardware audit (scripts + raw output)
research/  reverse-engineering notes (app IL, drivers zip, EC map)
```

## Verified on hardware (23/11/2026)

- `CTMP` (0xFE0B08B0) tracks load: 49 °C idle → 57 °C under 8-thread load.
- `RPM1`/`RPM2` (0x13/0x15) are **little-endian** 16-bit: ~2450/2750 rpm at idle,
  rising under load. Reading them big-endian gives nonsense (39 000 "rpm").
- `CTML` (0xFE0B08B1) is *not* the decimal part: it always equals `CTMP`.
- Keyboard RGB works from userspace (HID LampArray, reports 0x05/0x06).
- `BCPC` (0xFE0B0805) read **80** on this machine, matching the 80 % limit set in Windows.
  Cross-checked against the Linux driver `gigabyte-laptop-wmi` (WMI `0x65` = charge limit,
  `0x64` = charge mode with bit 2 in `BCPS`): **writing BCPC works**, verified 80 -> 60 -> 80
  from the CLI and from the app.
- End-to-end charging behaviour at 60 % (battery actually stopping there) still to be
  observed with a discharged battery.

## Dependencies

- **Keyboard RGB**: [gigabyte-kbd](https://github.com/Luminous418/Gigabyte-Gaming-A16-macOS-KB)
  (MIT, bundled inside `GiMATE.app/Contents/Resources`). One implementation of the
  LampArray protocol for both Linux and macOS.
- **Reverse-engineering reference**: [gigabyte-laptop-wmi](https://github.com/tangalbert919/gigabyte-laptop-wmi)
  (GPL-2.0). Used only as documentation for the WMI method IDs and EC fields; no code copied.

## License

MIT (to be added with the first code).
