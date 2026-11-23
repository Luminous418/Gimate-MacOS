#!/bin/bash
# Auditoría de solo lectura: qué expone macOS en este portátil.
OUT=/Users/lumi/Proyectos/Gimate-MacOS/audit
mkdir -p "$OUT"
cd "$OUT" || exit 1

sw_vers > 00-os.txt 2>&1
uname -a >> 00-os.txt 2>&1
sysctl -n hw.model machdep.cpu.brand_string >> 00-os.txt 2>&1

system_profiler SPHardwareDataType SPPowerDataType SPDisplaysDataType > 01-profiler.txt 2>&1
system_profiler SPSensorDataType >> 01-profiler.txt 2>&1
system_profiler SPUSBDataType > 02-usb.txt 2>&1

# Batería / carga
ioreg -rc AppleSmartBattery > 03-battery-raw.txt 2>&1
ioreg -rc AppleSmartBattery | grep -E '"(MaxCapacity|CurrentCapacity|DesignCapacity|CycleCount|Temperature|Voltage|Amperage|IsCharging|ExternalConnected|FullyCharged|ChargingCurrent|AppleRawCurrentCapacity|AppleRawMaxCapacity|ChargerConfiguration|BatteryData)"' > 03-battery.txt 2>&1

# SMC / sensores
ioreg -l -w0 | grep -iE 'AppleSMC|SMCSuperIO|SMCProcessor' | head -40 > 04-smc.txt 2>&1
ioreg -l -w0 | grep -oE '"[A-Za-z0-9]{4}" = ' | sort -u | head -400 > 04-smc-keys.txt 2>&1

# HID
ioreg -c IOHIDDevice -l -w0 > 05-hid.txt 2>&1
grep -ciE 'LampArray|0x59' 05-hid.txt > 05-hid-lamparray-count.txt 2>&1
ioreg -c IOHIDDevice -r -l -w0 | grep -B2 -A12 -i 'LampArray' | head -80 > 05-hid-lamparray.txt 2>&1

# Térmico
pmset -g therm > 06-therm.txt 2>&1
pmset -g thermlog >> 06-therm.txt 2>&1
pmset -g custom >> 06-therm.txt 2>&1
sysctl -a 2>/dev/null | grep -iE 'thermal|temp|fan|smc' | head -40 > 07-sysctl.txt 2>&1

# ACPI / EC
ioreg -l -w0 | grep -iE 'AppleACPIEC|ACPIEC|EC0' | head -20 > 08-acpi-ec.txt 2>&1

# Herramientas disponibles
for t in smc smctemp istats powermetrics nvidia-smi ioreg spctl kmutil; do
  printf '%-14s ' "$t"; command -v "$t" || echo "-"
done > 09-tools.txt 2>&1

# Kexts relevantes
kmutil showloaded --list-only | grep -iE 'SMC|NVRM|VirtualSMC|HID|MuxFix' > 10-kexts.txt 2>&1

echo OK
