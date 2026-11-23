# GIGABYTE "Drivers Gigabyte Gaming A16" bundle — read-only research

Date: 2026-11-23. Source: `/Volumes/Untitled/Users/Lumi/Downloads/Drivers Gigabyte Gaming A16.zip`
(NTFS, read-only mount, 12,241,997,983 bytes). Target machine: GA6H / i7-13620H / RTX 5050 Laptop.

Everything here was obtained **without executing any Windows binary** and without writing to the
NTFS volume. Work products live in `/Users/Shared/gimate-zip/` (≈15 GB; see §8 for what can be
deleted).

---

## 1. Tree of the ZIP, and what was extracted

The outer ZIP is only a container of 14 files (12.25 GB uncompressed):

| File | Stored size | Unpacked | Notes |
|---|---|---|---|
| `7z26.05-zstd-x64.exe` | 3.3 MB | — | Just the 7-Zip 26.05 tool shipped to the customer |
| `Gimate_Setup_All_Intel_260910.zip` | **10.17 GB** | 10.18 GB | The GiMATE / Control Center bundle (all the interesting stuff) |
| `nb-driver-64bit-win11-audio-dolby-6.0.9835.1_c.zip` | 546 MB | 792 MB, 439 files | Realtek+Intel SST APO / Dolby, `HDXGigabyteSST.inf`, `IntcAudioBus.sys` |
| `nb-driver-64bit-win11-dchu-bt-23.150.0.3_c.zip` | 17 MB | 25 MB, 11 files | `ibtusb.inf` / `ibtusb.sys` |
| `nb-driver-64bit-win11-dchu-chipset-10.1.36.7_c.zip` | 488 KB | — | 20 INF (`RaptorLakeSystem.inf`, `AlderLakePCH-PSystem*.inf`, …), **no WMI/ACPI entries** |
| `nb-driver-64bit-win11-dchu-dtt_ipf-20250401_c.zip` | 15 MB | 40 MB | Intel DTT + IPF, `dtt_sw.inf`, `dtt_ext.inf`, policy DLLs, `configuration/*.config` (binary, opaque) |
| `nb-driver-64bit-win11-dchu-gna-3.5.0.1578_c.zip` | 449 KB | — | `gna.inf` / `gna.sys` |
| `nb-driver-64bit-win11-dchu-me-2407.6.1.0_c.zip` | 247 MB | 437 MB, 286 files | Intel ME/AMT. **The only `.mof` files of the whole bundle are here** (see §4) |
| `nb-driver-64bit-win11-dchu-serialio-30.100.2417.30_c.zip` | 935 KB | — | `sio_*.exe` packaged installer |
| `nb-driver-64bit-win11-dchu-vga-intel-32.0.101.6556_c.zip` | 1021 MB | 2.6 GB, 329 files | iGPU DCH driver (`igdkmdn64.sys`, `igvk64.json`, …) |
| `nb-driver-64bit-win11-dchu-wlan-23.150.0.4_c.zip` | 37 MB | 101 MB, 31 files | Intel AX211 WiFi (`Netwtw6e.INF`, `Netwtw14.sys`, …) |
| `nb-driver-64bit-win11-lan-10.74.1128.2024_c.zip` | 1.1 MB | — | Realtek `rt640x64.inf/.sys` |
| `NVIDIA_app_v11.0.9.585_x64.exe` | 184 MB | — | NVIDIA App (not extracted) |

`Gimate_Setup_All_Intel_260910.zip` (27 entries):

```
CoreFrame.dll  HTTP_LIB.dll  Install_info.xml  Setup.exe
Package/ai/GimateAI/INTEL/FULL/GiMATE_ai_26.5.27.1.7z.001    6.6 GB  (LLM weights; NOT needed)
Package/ai/GimateAI/INTEL/FULL/GiMATE_ai_26.5.27.1.exe / .bat  version_control_intel.json  silent_install_*.bat
Package/AI.cmd  AIVisual_26.08.26.01.exe  BlockSleep.exe  driverinstall.cmd  imageinstall.cmd  Patch.bat
Package/GBT_NbSrv_26.09.07.01.exe                110 MB   <- notebook service + PgSvc + drivers
Package/GBT_rgbLaptop_UC_26.06.17.02.exe         200 KB   <- keyboard RGB
Package/GIGABYTE-GiMATE-Coder-26.415.1-setup.exe 660 MB   (LLM dev app)
Package/GIGABYTE-GiMATE-Creator-26.204.1-setup.exe 2.4 GB (LLM creator app)
Package/GiMATE_26.904.1.0.exe                    209 MB   <- the current GiMATE app (MSIX inside)
Package/GiMATE_Service_Image_26.08.28.02.exe     128 MB   <- GCC service + device.db/Device.ini
```

### How the payloads were unpacked (all in `/Users/Shared/gimate-zip/`)

All Gigabyte installers are **NSIS**; `unzip`/`bsdtar` cannot read them and macOS has no `7z`
installed, so a small NSIS (zlib, "solid") extractor was written in Python:

| Script | Purpose |
|---|---|
| `str16.py`, `stra.py` | ASCII / UTF-16 string dumps |
| `nsissolid.py` / `nsissolid2.py` | NSIS block extractor. Format: first block `[u32 usize][u32 ?][u32 0x80000000|csize][deflate]`, following blocks `[u32 0x80000000|csize][deflate]`, each chunk deflated **with the previous chunk's last 32 KB as preset dictionary** |
| `pever.py` | maps extracted blobs to original file names through PE `VS_VERSIONINFO` |
| `scan.py`, `ctx.py`, `cabscan.py`/`cabext.py` | keyword scans, one embedded CAB parser (unused in the end) |

Results:

* `msix/` — `GPilot_26.904.1.0_x64.msix` unpacked, **1 380 files / 186 MB**. This is GiMATE 26.904.1.0.
* `blocks_svc/` — 35 blocks from `GiMATE_Service_Image_26.08.28.02.exe` (34 of 35 identified).
* `blocks_svc2/` — 62 blocks from the nested `GiMATE Service_26.08.28.01.exe` (inside `blk018` of the above).
* `blocks_nb/` — first 17 blocks of `GBT_NbSrv_26.09.07.01.exe`.
* `extract/` — hand-picked evidence: `device.db`, `device_db.json`, `CalssMapJson*.txt`,
  `gdrv3.sys`, `GCC_main_GiMATEService.exe`, `GBT_Comm_Fun_svc.dll`,
  `GpuPowerGear_Hardware.dll` (= `AI_PowerGear_III.dll`), `IT8295_rgb.xml`, `FBIOS.dll`, `Flash.dll`,
  plus `Device.ini` text (from `blocks_svc/blk010`).

**Not extracted:** the 2.4 GB/660 MB LLM apps, the 6.6 GB `.7z.001` model, the 184 MB NVIDIA App,
and the 114 MB tail of `GBT_NbSrv_26.09.07.01.exe` (see §7, "gaps").

---

## 2. Inventory of applications, services and drivers

### 2.1 GiMATE app (MSIX) — `GiMATE_26.904.1.0.exe` → `msix/`

* `AppxManifest.xml`: `Identity Name="GiMATE" Version="26.904.1.0" x64`, publisher
  `CN="GIGA-BYTE TECHNOLOGY CO., LTD."`, `GiMATE.runtimeconfig.json` → **.NET 8 / WinUI 3**
  (`Microsoft.WinUI.dll`, `Microsoft.WindowsAppRuntime.1.8.msix` + `windowsdesktop-runtime-8.0.10-win-x64.exe`
  are installed by the bootstrapper `AppInstallCmd.exe -bc | -si | -rc | -pin-restart`).
* Binaries: `GiMATE.exe` (308 KB), `GiMATE.dll` (2.68 MB), `GPilot.Core.dll`, `Gpilot_Chat_Lib.dll`,
  `midleware.dll` (274 KB, native C++ — Win32 power APIs: `PowerGetActiveScheme`,
  `_POWER_LIMIT_TYPES`, `_DEVICE_POWER_STATE`…), `GBT_Comm_Fun.dll` (42 KB, **the WMI wrapper**, §3),
  `System.Management.dll`, `LiveChartsCore*`, `SharpDX*`, `Microsoft.ML.OnnxRuntime.dll`,
  `CommunityToolkit.WinUI.*`, `Newtonsoft.Json`, `Serilog`.
* Assets: `Assets/Laptop/{Power,Fan,SystemPerformance,Keyboard,Display}/…`,
  `Assets/RGBF30/{PatternIcon,devIcon}/…`, `Assets/Modes/{eco,balance,game}.svg`.

### 2.2 GCC / "GiMATE Service" — `GiMATE_Service_Image_26.08.28.02.exe`

Installs into `C:\Program Files\GIGABYTE\Control Center\` and drops a scheduled task
(`GiMATE Service.xml`) plus `GimateServiceHelper.xml`. The file list (from the installer header)
is the authoritative component list — 34 blocks matched by PE version info:

| File | Version | Role |
|---|---|---|
| `GCC.exe` (**`GCC_main_GiMATEService.exe`**, 34 MB, `ProductName=GiMATEService`) | 25.12.16.1 | Main Control Center host, **named-pipe server** (`Create_PipeServer`, `Pipe_Fun_Class`, `Pipe_Server`) |
| `GBT_Comm_Fun.dll` | 26.02.11.01 / 22.10.05.01 | WMI + capability wrapper (§3) |
| `GCC_Main_COMM_Fun.dll` | 23.04.07.1 | older IPC/WMI helpers |
| `GvLoader.dll` (`GvDevice`) | 23.12.18.01 | device enumeration |
| `Search_Device_Lib.dll` | 23.10.21.1 | HW-ID → capability lookup |
| `ExtLIB_Loader_Lib.dll` | 24.10.22.1 | loads `Lib\<component>\*.dll` |
| `GHidApi.dll` | 24.9.9.1 | HID access (RGB, macro keys) |
| `RgbCommon.dll`, `RGBFI.dll`, `UIEffect.dll`, `Pattern_*_Usercontrol.dll` | — | RGB engine + UI |
| `Gigabyte.IC.Profile.dll`, `MB.CPNT.dll`, `YccDrvv3.dll` | — | profiles, panels, **user-mode wrapper of `gdrv3`** |
| `GBT_RPC_Library.dll` | — | RPC |
| `HTTP_LIB.dll` | 22.6.17.1 | updates |
| `InstUpd.exe` | 25.08.05.01 | installer updater (`Software\...\App Paths\InstUpd.exe`, `-NORUN`) |
| `FBIOS.dll`, `Flash.dll`, `FlashBios.exe`, `dmLib.dll` | 1.0.0.1 | **BIOS flash** (see §6) |
| `GimateServiceHelper.exe`, `LaunchGCC.exe`, `Uninstall_Package.exe`, `uiall.exe` | — | helpers |
| `System.Data.SQLite.dll`, `SQLite.Interop.dll` | 1.0.118 | reads `device.db` |
| `device.db` (SQLite), `Device.ini`, `CalssMapJson.txt` | — | capability tables (§5) |
| `Microsoft.Win32.TaskScheduler.dll`, `WindowsFirewallHelper.dll` | — | task + firewall rule |

### 2.3 Notebook service — `GBT_NbSrv_26.09.07.01.exe`

Installs to `C:\Program Files\GIGABYTE\Control Center\Lib\GBT_NbSrv\` and
`...\Lib\GBT_Notebook\`. Header file list (extraction verified for the first 17 blocks):

* `GBTService.exe` (launched as `-B1`, `-work`, `-assign`), `GBT_NbSrv.dll`, `GBT_Notebook\ucNotebook.dll`
  (class `ucNotebook.NB_Controler` — **the notebook ECU layer**), `Gigabyte.NB.Devices.dll`.
* **`PgSvc\` — the AI Power Gear III suite**: `Gbt.GpuPowerGear.{Service,UI,Proxy,Comm,Comm.Gcc,Comm.PrimarySvc,Comm.Proxy,Config,Hardware,Utilities,UserInteraction}.dll`,
  `…Proxy.exe --magic=GiGABYTE`, `…Service.exe`, `…UI.exe`, `Gbt.GpuPowerGear.Proxy.deps.json`, etc.
  Only `AI_PowerGear_III.dll` was recovered from the extractable part
  (`extract/GpuPowerGear_Hardware.dll`, PE `ProductName=AI_PowerGear_III`,
  `FileVersion=26.8.5.1`, PDB
  `C:\Users\gbt\source\repos\GbtGpuPowerGearSuite-master\Gbt.ToastNotifier\obj\Release\net8.0-windows10.0.22000.0\AI_PowerGear_III.pdb`).
* **`acpimof.dll`** — present in this package (ACPI-MOF helper, see §4).
* `AI_PowerGear_III.exe / .dll / .resources.dll` (26.8.5.1).
* Firmware/EC tools: `ITE_PSU.exe` + `PS8483_Generic_4_1_0_3.bin` + `bootloader_04.bin`,
  `GbtBiosAidCli.exe`, `GbtFwAgent.exe`, `GprocSrvc.exe`, `OSDwindow.exe`, `io.dll`,
  `GbtNbActivation.exe` (`\ActTools\GbtNbActivation.exe --install`), `GbtNbActivationAgent*.exe`,
  `CPUOC.dll`, `CoreX15.dll`, `CoreX16.dll`, `ComData.dll`, `CoreFrame.dll`, `nsExec.dll`.
* `DMWork.exe` / `dmWork.exe` is used by the RGB package to kill/replace GCC.

### 2.4 Keyboard RGB — `GBT_rgbLaptop_UC_26.06.17.02.exe` (NSIS 3.08, LZMA — fully decoded)

Registry `Software\Microsoft\Windows\CurrentVersion\Uninstall\GBT_rgbLaptop_UC`,
`...\App Paths\GBT_rgbLaptop_UC.dll`, installs `Lib\GBT_rgbLaptop_UC\`:
`GBT_rgbLaptop_UC.dll`, `RgbLaptop.dll`, `Dynamic_Lighting_Fun_Lib.dll`, `RgbCommon.dll`,
`GHidApi.dll`, `SMBCtrl.dll`, and profile data:
**`GbtFusionData\NbInfo.json`** plus `Profile1.json … Profile5.json` (`Profile{0}.json`).
`dmWork.exe -kill GCC.exe` is used while updating.

### 2.5 Windows drivers in the bundle (for the record)

`gdrv3.sys` (see §3), `ibtusb.sys` (BT), `Netwtw*.sys` (WiFi), `rt640x64.sys` (LAN),
`igdkmdn64.sys` (iGPU), `IntcAudioBus.sys` (SST/Dolby APO), `dtt_sw`/`dptf*` (Intel DTT),
`gna.sys`, ME (`TeeDriver`, AMT WMI providers).

---

## 3. WMI / ACPI / EC — the important part

### 3.1 The app-side WMI wrapper: `GBT_Comm_Fun.dll`

`.NET` assembly, PDB `D:\Project\GCC_SW\GBT_Comm_Fun\obj\Release\GBT_Comm_Fun.pdb`.
Every string below is literal metadata (TypeDef/MethodDef/property names) from the DLL:

```
GBT_WMI_Method_delegate          GBT_WMI_Event_delegate        GBT_Capability_Event_delegate
<WMI_Data>k__BackingField        <WMI_Dictionary>k__BackingField
<WMI_Number>k__BackingField      <WMI_String>k__BackingField   <WMI_Type_Name>k__BackingField
IGBT_WMI_NB_Interface            IGBT_WMI_NOTIFY_NB_Interface
IGBT_NB_WMI_Method_Data_Interface  IGBT_NB_WMI_Notify_Data_Interface
NB_WMI_Method_Data               NB_WMI_Notify_Data
IGBT_Device_Interface  IGBT_Device_Capability_Desp_Interface  IGBT_Device_Sync_Interface
IGBT_Usercontrol_Interface  IGBT_UWP_Capability_interface  IGBT_UI_Control_Fun_Interface
Get_Device_Capility  Get_All_Support  Get_SetCapability  Execute_Capility_Fun
Get_Cap_args_json_string  Set_Cap_args_json_string  Get_Json_String  From_Json_String
Capability_Keys  Category_ID_Enum  Device_Capability_Desp  Device_Sync_Info  Support_Flag
```

Capability enumeration (verbatim): `Cap_General`, `Cap_Dashboard`, `Cap_FanControl`,
`Cap_Performance`, `Cap_RGB`, `Cap_AmbientLED`, `Cap_LCDPanel`, `Cap_KeyBoardTool`, `Cap_Macro`,
`Cap_MouseTool`, `Cap_DPSU`, `Cap_Recovery`, `Cap_SsdTool`, `Cap_Update`, `Cap_HwInfo`,
`Cap_Sidekick`, `Cap_key`. Also `CpuCooler`, `FanBox`, `VgaCard2Fan`, `WaterCoolingFan`, `LedPad`,
`GamingBox`, `AllinOne`, `Aorus`, `NoteBook`, `Monitor`, `Keyboard`, `Mouse`, `Headphone`,
`MDot2SSD`, `PCIeSSD`.

> **Note:** the *class names* of the ACPI-WMI interface (`GB_WMIACPI_Get/Set/Data/Event`) do **not**
> appear anywhere in these binaries — not in `GBT_Comm_Fun.dll`, not in `GiMATE.dll`, not in `GCC.exe`.
> They are built at run time (the ACPI MOF side, exposed by `AMW0`/`PNP0C14`), which is why the string
> search comes up empty. `acpimof.dll` in `GBT_NbSrv` is the piece that deals with the MOF.

### 3.2 `gdrv3.sys` — the ring-0 door to the EC (the single most useful find)

Present **twice** in the GiMATE Service installer (`extract/gdrv3.sys`, `extract/gdrv3_copy.sys`,
265 896 and 252 528 bytes) with PE info `GIGA-BYTE Nonpnp Driver`, `ProductName=GIGA-BYTE Software
Driver`, `FileVersion=1.2.0.1`, KMDF. PDBs: `D:\Projects\gdrv\x64\Release\gdrv.pdb`,
`D:\Projects\ycc64\x64\Release\YccDrvv3.pdb` (this is the well-known Gigabyte `gdrv` toolbox driver,
v3 branch). Strings, exact:

```
\Device\GIOV3            \DosDevices\GIOV3            \\.
\system32\drivers\gdrv3.sys   \gdrv.sys   \gdrv2.sys
access_io_port           CheckNewSMI_State            CONTROL_DEVICE_EXTENSION
Intel 300/400 EC switch address.
AMD beatmode memory address
MCFG address             Skylake Base address          Fixed MMIO memory
Steven Fixed MMIO memory   SuperIO chip 0x%x
MailBox addr is 0x%llx    Mapping addr addr is 0x%llx   New SMI address
LockAL is %x              size is 0x%x
\??\c:\gmem.dump          ExGetFirmwareEnvironmentVariable
```

Interpretation (supported by the strings, consistent with the DSDT map in
`research/macos-audit.md`): on Windows, GiMATE/GCC reaches the **EC page at physical `0xFE0B0800`**
not through ACPI but by opening `\\.\GIOV3` and asking this KMDF driver to **map fixed MMIO /
read-write I/O ports** (`access_io_port`), plus SMBus/EC mailbox helpers and an
"Intel 300/400 EC switch address" workaround. `YccDrvv3.dll` in the GCC service is the user-mode
wrapper of the same IOCTL interface.

Consequence: the Windows architecture is **two-lane**:
* **lane A (ACPI-WMI)** — `AMW0` → `WMBC`/`WMBD`, `Arg1 = WmiMethodId` (0xE6 MUX, 0x64/0x65 BCPS/BCPC,
  0x71/0x70 fans, 0xF6 keyboard light, …). Used for things that must survive sleep / need firmware
  policy. This is what `Gbt.GpuPowerGear.Hardware.dll` calls.
* **lane B (raw EC)** — `gdrv3.sys` mapping `0xFE0B0800` + port I/O + SMBus/ITE. Used for the
  live sensor/fan-table pages and the ITE RGB/PSU chips.

For a macOS port this matters: lane B has **no equivalent** (no third-party kernel driver may map
that page casually, and it is exactly the kind of thing that must be a kext), while lane A is
already reachable on the ACPI side if we ever needed to emulate the WMI calls (we cannot — there is
no ACPI-WMI host on macOS; the `_DSM`-style path is dead).

### 3.3 The UI ↔ service protocol (JSON over a named pipe)

The modern app does **not** call WMI directly for notebook functions; it sends JSON to the GCC
service through a named pipe (`GPilot.Helpers.IpcHelper.SendMessageToNamedPipeServer`,
`NamedPipeClient`, `SendToAssignPipeServerAsync`, `PipeName`). Command templates are literals in
`GiMATE.dll` (UTF-16), 42 template lines, of the form:

```json
{"Device_ID":0,"Ext_Desp":[{"Key":"WPF_CLASS","Value":"ucNotebook.NB_Controler"},
                           {"Key":"AiPowerGear","Value":"Mode"},
                           {"Key":"ModeSetting","Value":"3"},
                           {"Key":"ResetMode","Value":"0"}],"FunctionPage":1}
```

Observed groups and values (verbatim):

| `Key` | `Value` | `FunctionPage` |
|---|---|---|
| `AiPowerGear` | `Mode` + `ModeSetting` 0/1/2/3 + `ResetMode` 0/1 | 1 |
| `AiPowerGear` | `QueryComputeApps` (`Index` 0/1), `StopProcess` (`PIDs`) | 1 |
| `GeneralSetting` | `ScenarioMode`, `X3dTurboMode`, `CheckNvidia`, `NvidiaApp`, `NVCtrlPanel`, `GbtFd`, `GiMATECoder`, `GiMATECreator`, `Reboot` (`Setting`=3) | 1 |
| `Dashboard` | `DashboardRequest` | 2 |
| `InfoRequest` | `1` | 8 |
| `Gpilot` | `RgbFusionInit` | 16 |
| `RecoverySetting` | `Initial`, `CheckStatus`, `CheckWIM`, `DownloadWIM`, `Create` (`DiskID`), `Cancel*` | 32 |
| `MB.CPNT.Internal.Storage.StorageGroupPanel` | `GPILOT:INITIAL`, `Read` | 2048 |
| `AiBoost` | `OnOff` + `Setting` | 65536 |
| `GBT_WINUI_Bridg_UC.GBT_WINUI_Bridg_Control` | `1` / `2` / `{0}` | 0 |

`Device_ID:0` = the notebook itself; `Ext_Desp` is the ordered key/value list; `FunctionPage` is a
bitmask of the UI page owning the request. The reply carries `WMI_Number` / `WMI_String` /
`WMI_Data` / `WMI_Dictionary` and `WMI_Type_Name`, i.e. the pipe payload is a serialization of the
same `NB_WMI_Method_Data` structure used for the ACPI method call — so the ACPI WMI call is
performed by the **service**, and the app only speaks JSON.

### 3.4 Device → panel mapping tables

* `device.db` (SQLite, `extract/device.db`): one table `devicelist(info TEXT)` with a single JSON
  document, **654 keys**, all of the form `VENID_DEVID_SSID_SVID` (e.g. `10DE_1458_41C5_2D04`,
  `0414_7A5F`) → `{ModelName, MKTName, Version, UiId, DefaultEffect, DefaultColorArray}`.
  This is the **GPU/AIC name table** (RTX 50xx + Gigabyte part numbers) used to label the VGA page,
  not the laptop feature table.
* `Device.ini` (`blocks_svc/blk010`, 7 KB, INI) — 28 sections `[VID_PID]` with
  `ModelName`, `MKTName`, `ShortName`, `DeviceType` (0…7) for Gaming Box / header / chassis / PSU /
  cooler / keyboard / mouse. `DeviceType` is the peripheral category index.
* `CalssMapJson.txt` (18 KB, `extract/CalssMapJson.txt`) — one JSON object per line mapping
  `Class_name` → `HW_ID`, e.g. `{"Class_name":"ucMonitor.Sidekick","HW_ID":"MONITOR\\GBT"}`,
  `{"Class_name":"MB.CPNT.Internal.LED.DramPanel","HW_ID":"System\\Product_Name\\Z890"}`.
  The notebook rows are `ucNotebook.NB_Controler` (lib `GBT_Notebook`), `GBT_rgbLaptop_UC.ucRGB_Laptop`.
* `CalssMapJson2.txt` (`blk034`) — `Class_name` → `LIB` (which DLL must be installed for the panel:
  `MBStorage`, `MBEasyTune`, `GBT_rgbMonitor_UC`, `GBT_Notebook`, `Sidekick`, …).
* `IT8295_rgb.xml` (`blocks_svc2/blk030`) — the ITE IT8295 RGB controller animation tables
  (`<Breathing><sp num="0" wtBase0="1600" …`), i.e. the LED engine's parameter set.

**There is no per-model capability file (the famous `NbInfo.json`) for the GA6H in this bundle.**
`NbInfo.json` ships per model inside the *RGB* package `GbtFusionData\NbInfo.json` and is
regenerated/updated at runtime; the app logs
`[rgbf30_msg1058]: NbInfo.json file not found at: `, `[rgbf30_msg5880]: Failed to deserialize NbInfo.json.`,
`[rgbf30_warr5726]: MainWindow(), failed to load NbInfo or not a RGB model.` This is the file worth
grabbing from the Windows install (§7).

---

## 4. `.mof` and WMI schema files — answer to the "gold" question

* **No Gigabyte `.mof` exists anywhere in the bundle.** The only `.mof` files are the standard Intel
  ME/AMT providers inside `nb-driver-64bit-win11-dchu-me-2407.6.1.0_c.zip`
  (`AMT_*.mof`, `CIM_*.mof`, ~280 files) — Microsoft/Intel CIM classes, unrelated to `GB_WMIACPI`.
* The Gigabyte WMI classes come from the **ACPI namespace** (the DSDT/SSDT of the machine, already
  mapped in `audit/`), therefore no `.mof` is needed on the Windows side either: Windows builds the
  `GB_WMIACPI_*` WMI classes dynamically from the ACPI MOF (`acpimof.dll` present in `GBT_NbSrv`).
* `.inf` files: no `AddReg`/WMI provider was found in the chipset/DTT/GNA/LAN/WLAN/BT INFs
  (they are the usual Intel/Realtek DCH packages). No INF installs a WMI provider.

So the WMI method table can only be derived from the **DSDT** (already done) and from the
`WmiMethodId` values used by `Gbt.GpuPowerGear.Hardware.dll` (already decompiled). The ZIP confirms
the plumbing but adds no new IDs.

---

## 5. Battery charge limit, fans, power modes (with evidence)

### 5.1 Battery charge limit — the feature exists and is a 3-state selector

From `msix/GiMATE.dll` (UTF-16 and metadata strings):

```
Battery_Charge        Battery_Charge_Stop       Battery_Charge_Stop60      Battery_Full
ChargeIdx             ChargeMode                ChargeRate                 IsChargeEnabled
ChargeSelectionChange: Send message to named pipe server failed
GPilot.Controls.Laptop.HardwareSetting.PowerOptionsExpander.ChargeIdx
GPilot.Controls.Laptop.HardwareSetting.PowerOptionsExpander.IsChargeEnabled
UcPower_ChargeMode    PowerPageStatusChargeMode  PowerPageStatusChargeModeTitle  ChargeModeBox
ChargeOp0             ChargeOp1                 ChargeOp2
/Assets/Laptop/Power/Icon/BatteryCharge.svg
```

Reading: the Power page has a "battery charge" expander with a toggle (`IsChargeEnabled`) and a
selector with **three options** (`ChargeOp0/1/2`, bound to `ChargeIdx`), one of which is explicitly
"stop at 60 %" (`Battery_Charge_Stop60`) and one "full" (`Battery_Full`); the change is pushed to the
service over the named pipe, not through WMI directly. In the DSDT the matching objects are
`BCPS` (`0xFE0B0803`, 4 bits) and `BCPC` (`0xFE0B0805`, 8 bits), addressed through WMI IDs
`0x64`/`0x65` — consistent with a "mode/stage + percentage" pair, i.e. `BCPS` = mode
(0 normal, 1 stop, 2 stop-at-x) and `BCPC` = threshold. **Not verified behaviourally** — needs one
run of GiMATE on Windows with the EC page dumped before/after.

### 5.2 Fans

```
Cap_FanControl     FanBox     VgaCard2Fan     WaterCoolingFan
"FanMode"          /Assets/Laptop/Fan/{Frame 4531.svg, Frame 4533.svg, Icon/Mode.svg}
/AiPowerGearIII: AI Power Gear II / III, AI_PowerGear_III.dll (26.8.5.1)
```

Fan control is a *capability* (`Cap_FanControl`) resolved per device by `GBT_Comm_Fun`; the actual
curve tables live in the EC (`TFAN 0xFE0B080B`, `GFAN 0xFE0B080C`, `CRAF/FANB 0xFE0B082C`,
`FAN1/FAN2 0xFE0B081B/1C`) and in `ucNotebook.dll`, which was not extractable (see §7).
The two ITE PS8483 firmware blobs in `GBT_NbSrv` (`PS8483_Generic_4_1_0_3.bin`, `bootloader_04.bin`)
are the **PSU/EC** firmware of the notebook, not fan curves.

### 5.3 Power / performance modes

* `AiPowerGear` `Mode` with `ModeSetting` **0/1/2/3** and `ResetMode` 0/1 (see §3.3) — four power
  profiles, one of them flagged "reset".
* Assets: `Assets/Modes/{eco,balance,game}.svg`, `PowerGear_mode_{auto,eco,optimus}.svg`,
  `SystemPerformance/Icon/sysperf_{balance,silent}_{default,disable,selected}.svg`
  → UI exposes **eco / balance / game(performance) / auto / optimus** plus silent/balance
  performance modes, and "AI Power Gear II/III" branding.
* `midleware.dll` links `PowerGetActiveScheme` / `PowerSetActiveScheme` family (Windows power plans)
  → power-mode changes are **dual**: Windows power scheme + EC/ACPI (via the pipe to the service).
* MUX / GPU switching is a separate control: `OnSetDgpuConfigResult`,
  `IsDgpuOnly`, `CheckDGpuOnlyMode`, `dGpuMode4DiscreteLayoutGrid` / `dGpuMode4DynamicLayoutGrid`,
  `DgpuStatusComboBox`, and the literal log
  `{TXT_LOG_PREFIX}: OnSetDgpuConfigResult - Because NVIDIA DDS is enabled and settings are blocked. (-2)`
  → the "Discrete / Dynamic(Optimus)" switch in the current app goes through **NVIDIA DDS**, on top
  of the BIOS/EC `0xD0` mechanism already replicated by `muxfix`.

---

## 6. Firmware / BIOS utilities — do NOT run these

| Utility | Package | Why not |
|---|---|---|
| `FlashBios.exe`, `FBIOS.dll`, `Flash.dll`, `dmLib.dll` | GCC service (`Lib\Update_Center`) | BIOS flash from Windows |
| `GbtBiosAidCli.exe` | `GBT_NbSrv` | BIOS "aid" CLI (flash / settings) |
| `GbtFwAgent.exe` | `GBT_NbSrv` | firmware agent (background flash service) |
| `ITE_PSU.exe` + `PS8483_Generic_4_1_0_3.bin` + `bootloader_04.bin` | `GBT_NbSrv` | EC/PSU firmware + bootloader **for the ITE chip** — wrong write = dead board |
| `GbtNbActivation.exe --install`, `GbtNbActivationAgent*.exe` | `GBT_NbSrv\ActTools` | activation/licensing agent (installs services) |
| `Setup.exe` (root of the bundle) + `Install_info*.xml` | bundle root | update bootstrapper; downloads from `https://mb.download.gigabyte.com/FileList/Swhttp/` (and a dev URL `http://10.1.9.82/Swhttp/`), kills GiMATE/GCC and rewrites `EFI/OC` if a BIOS/NullMoth package is involved — **same class of updater that already wiped `ForbiddenItlwm`/`MuxFix` from the EFI twice** |
| `GiMATE_ai_*.bat`, `driverinstall.cmd`, `Patch.bat`, `imageinstall.cmd` | bundle | unattended installers: `rd /s /q` of program folders, scheduled tasks, `taskkill`, reboot flags |

Nothing in the bundle is a BIOS *image*; these are only the flash *tools*, which is why the warning
is "never run", not "never read".

---

## 7. Gaps / what is still missing

1. **114 MB of `GBT_NbSrv_26.09.07.01.exe` could not be unpacked.** The NSIS zlib-solid chain breaks
   at file offset `796 929` (after 17 blocks) and the rest is uniformly high-entropy (7.97 bit/byte),
   with no embedded `MZ`, `PK`, `MSCF`, `BZh` or LZMA signature. That tail is where
   `ucNotebook.dll`, `acpimof.dll` and most of the `Gbt.GpuPowerGear.*` suite live.
   Workarounds: (a) run `7z x` (the bundle ships 7-Zip 26.05, and 7-Zip has a native NSIS handler)
   on a Windows box or VM; (b) copy the installed tree from Windows
   (`C:\Program Files\GIGABYTE\Control Center\Lib\GBT_NbSrv`).
2. **`NbInfo.json` (per-model feature flags)** — not in the bundle; must be taken from a real
   Windows install (`...\GIGABYTE\Control Center\Lib\GBT_rgbLaptop_UC\GbtFusionData\NbInfo.json`)
   together with `Profile1..5.json`.
3. **No behavioural confirmation** of `BCPS`/`BCPC` (charge limit) nor of the `ModeSetting` 0…3
   mapping to eco/balance/performance/game. Both need one Windows session with EC dumps.
4. `Device.ini`'s `DeviceType` legend (0…7) is not documented inside the file.
5. Intel DTT policy files (`configuration/*.config`) are opaque binaries (not INI/XML) — no readable
   power policy, so nothing to port from there.

---

## 8. Suggested next steps

1. **Don't chase WMI on macOS.** There is no ACPI-WMI host; the EC page `0xFE0B0800` was only ever
   reached on Windows through `gdrv3.sys` (MMIO map) or through firmware (`WMBD`). Our kext
   (`kext/GiMATEEC.kext`, read-only) is the only sane replacement — keep it read-only first.
2. **Charge limit**: read `BCPS`/`BCPC` from the EC on the Hackintosh *before* writing anything
   (`tools/gmec`), then correlate with `Battery_Charge_Stop60` / `ChargeOp0..2`: expect
   `BCPS ∈ {0,1,2}` and `BCPC ∈ {0,60,80,100}`. Only then add a write path, and only if the EC
   keeps the value across reboot (the Windows UI implies persistence via the service).
3. **Ask for three files from the Windows side** (they are the remaining unknowns):
   `NbInfo.json`, `ucNotebook.dll`, `acpimof.dll` — or simply run `7z x GBT_NbSrv_26.09.07.01.exe`.
4. **Protocol reuse**: the JSON `Ext_Desp` + `FunctionPage` schema (§3.3) is a clean spec for our
   app's internal API — mirror it (`device_id`, ordered key/value, page bitmask) so the UI logic that
   is already known from `GiMATE.dll` maps 1:1.
5. **MUX**: keep `muxfix` as-is; the app should only *display* state and optionally call the same
   `0xD0` write, never through NVIDIA DDS (Windows-only API).
6. Cleanup: `/Users/Shared/gimate-zip/{raw,gimate,more}` (~14.7 GB) are only extraction by-products;
   keep `msix/`, `blocks_*`, `extract/` and the scripts (`nsissolid2.py`, `pever.py`, `str16.py`,
   `stra.py`) — they are what makes this repeatable.

---

## 9. Files worth opening first

| Path (in `/Users/Shared/gimate-zip/`) | Why |
|---|---|
| `extract/gdrv3.sys` | EC MMIO/port-I/O door — the Windows equivalent of our kext |
| `extract/GBT_Comm_Fun_svc.dll` | full WMI/capability API surface (§3.1) |
| `msix/GiMATE.dll` | 42 JSON command templates, charge/fan/MUX UI strings |
| `extract/device.db`, `extract/device_db.json` | 654 GPU/AIC name entries |
| `extract/CalssMapJson.txt`, `CalssMapJson2.txt` | class ↔ HW_ID ↔ library matrix |
| `extract/GCC_main_GiMATEService.exe` | named-pipe server side of the protocol |
| `blocks_nb/blk000_00069140.bin` | `GBT_NbSrv` file list (what the tail hides) |
| `blocks_svc/blk010_00680426.bin` | `Device.ini` (peripheral types) |
