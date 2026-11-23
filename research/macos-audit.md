# macOS hardware audit — GIGABYTE GAMING A16 (GA6H)

Date: 2026-11-23. macOS 26.7.3 (25G342), i7-13620H, RTX 5050 (NullMoth 3.0).

Everything below is **read-only** findings. Scripts in `audit/`.

## 1. What macOS exposes by itself

| Area | Result |
|---|---|
| **Keyboard RGB** | ✅ Reachable. `hidutil list` shows VID `0x414` PID `0x8105`, and a second node with **UsagePage 89 (0x59) / Usage 1** = **HID LampArray** (`AppleUserHIDDevice`). Pure userspace, no kext — same protocol as [gigabyte-kbd](https://github.com/Luminous418/Gigabyte-Gaming-A16-macOS-KB) |
| **Battery** | Read-only: `AppleSmartBattery` gives `MaxCapacity` 4890, `CurrentCapacity`, `CycleCount` 55, `Temperature` (0.1 K), `ChargerData`. **No charge-limit API** |
| **CPU temperature via SMC** | ❌ `powermetrics --samplers smc` → `Invalid key detected`. A direct AppleSMC probe (`audit/smc_probe.c`) returns `size=0` for `TC0P`, `TC0D`, `TG0P`… → **VirtualSMC/SMCProcessor exposes no temperature keys on this CPU** |
| **Fans via SMC** | ❌ `FNum = 0` (no fan keys) |
| **Thermal warnings** | `pmset -g therm` → *"No thermal warning level has been recorded"* |
| **Display / GPU** | Panel on the NVIDIA GPU (`NVRMDisplay` present) when the MUX is Discrete |

So macOS gives us **no fan speed, no CPU temperature, no charge limit**. The only thing usable
directly is the keyboard RGB.

## 2. The EC window (what GiMATE uses)

`OperationRegion (ECMM|PECM, SystemMemory, 0xFE0B0800, 0x1000)` — two Fields over the same
page. Offsets below were extracted from the DSDT (`audit/ec_map.py`).

### Sensors — read-only, safe

| Field | Address | Width | Guess |
|---|---|---|---|
| `RPM1` | `0xFE0B0813` | 16 | fan 1 RPM |
| `RPM2` | `0xFE0B0815` | 16 | fan 2 RPM |
| `CTMP` | `0xFE0B08B0` | 8 | CPU temp, integer part |
| `CTML` | `0xFE0B08B1` | 8 | CPU temp, decimals |
| `FAN1` | `0xFE0B081B` | 8 | fan 1 duty (%) |
| `FAN2` | `0xFE0B081C` | 8 | fan 2 duty (%) |
| `FLVL` | `0xFE0B084B` | 8 | fan level / mode |
| `KBLL` | `0xFE0B0831` | 8 | keyboard backlight level |

### Flags

| Field | Address | Bit | Guess |
|---|---|---|---|
| `TFAN` | `0xFE0B080B` | 7 | fan table / mode |
| `GFAN` | `0xFE0B080C` | 1 | fan governor on |
| `CRAF` / `FANB` | `0xFE0B082C` | 0 / 1 | custom ramp / fan boost |
| `PSON` | `0xFE0B0807` | 1 | power state |
| `BLED` / `PLED` | `0xFE0B0807` | 5 / 7 | battery / power LED |

### Battery charge (candidates, to be confirmed against GiMATE)

| Field | Address | Width |
|---|---|---|
| `BCPS` | `0xFE0B0803` | 4 bits |
| `BCPC` | `0xFE0B0805` | 8 |

## 3. WMI / ACPI

Device `AMW0` (`PNP0C14`, `_UID "DCK"`), classes `GB_WMIACPI_Get/Set/Data/Event`.
Dispatch: `WMBC` (read) / `WMBD` (write), `Arg1` = `WmiMethodId`.

Confirmed so far:

| ID | Meaning |
|---|---|
| `0xE6` | display mode (`SetPEG2orSG2`) → CMOS `0xD0` (ports `0x72/0x73`): 0 MSHybrid, 1 Discrete, consumed at POST |
| `0x7D` | `TFAN` |
| `0x71` / `0x70` | `FANB` / `FAN1`+`FAN2` |
| `0x57` | `CRAF` |
| `0x67` | `TENF` |
| `0x66` | `FLVL` |
| `0x80` | `PL3E` |
| `0xF6` | `KBLL` |
| `0xED` | `PFMM` (0..n profile?) |
| `0x64` / `0x65` | `BCPS` / `BCPC` |

## 4. Consequences for the app

1. **RGB**: userspace, do it directly with IOKit HID (no kext).
2. **Sensors (fans, temps)**: impossible from userspace → need a **read-only kext** that maps
   `0xFE0B0800` and exposes the values (sysctls, like `debug.gimate.*`).
3. **Charge limit**: needs writing `BCPS`/`BCPC` (or the matching WMI IDs) → same kext, but
   only after confirming the semantics against GiMATE on Windows.
4. **MUX**: already solved in [muxfix](https://github.com/Luminous418/muxfix); the app only
   needs to display the state (panel on NVIDIA vs Intel).

## 5. Next

- `kext/`: `GiMATEEC.kext` read-only, exposing RPM1/RPM2/CTMP/CTML/FAN1/FAN2/FLVL/KBLL.
- `tools/gmec`: CLI to verify the values against reality (spin the fans, watch them move).
- `app/`: SwiftUI menu-bar app: sensors + RGB + MUX state.
- Then: charge limit, after the Windows dumps.
