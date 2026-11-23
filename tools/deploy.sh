#!/bin/bash
# Despliega GiMATEEC igual que MuxFix: kext en la EFI + copia en L/E + LaunchDaemon.
set -e
OUT=/Users/Shared/gimate-deploy.txt
K=/Users/lumi/Proyectos/Gimate-MacOS/kext
DEV=$(diskutil list | awk '/EFI NO NAME +4\.3 GB/ {print $NF; exit}')
{
  echo "ESP: $DEV"
  diskutil mount "$DEV" >/dev/null
  M="/Volumes/NO NAME"

  echo "=== copia de la EFI (solo config, por si acaso) ==="
  TS=$(date +%Y%m%d-%H%M%S)
  mkdir -p /Users/Shared/gimate-backup-$TS
  cp "$M/EFI/OC/config.plist" /Users/Shared/gimate-backup-$TS/config.plist.bak
  echo "backup: /Users/Shared/gimate-backup-$TS"

  echo "=== kext en EFI ==="
  rm -rf "$M/EFI/OC/Kexts/GiMATEEC.kext"
  cp -R "$K/GiMATEEC.kext" "$M/EFI/OC/Kexts/"
  find "$M/EFI/OC/Kexts/GiMATEEC.kext" -type f

  echo "=== entrada en config ==="
  python3 /Users/lumi/Proyectos/Gimate-MacOS/tools/add_kext_entry.py \
      GiMATEEC.kext Contents/MacOS/GiMATEEC "GiMATEEC: read-only EC sensors"
  plutil -lint "$M/EFI/OC/config.plist"

  echo "=== copia en /Library/Extensions ==="
  rm -rf /Library/Extensions/GiMATEEC.kext
  cp -R "$K/GiMATEEC.kext" /Library/Extensions/
  chown -R root:wheel /Library/Extensions/GiMATEEC.kext
  chmod -R 755 /Library/Extensions/GiMATEEC.kext

  echo "=== LaunchDaemon ==="
  cp "$K/com.lumi.gimateec.plist" /Library/LaunchDaemons/
  chown root:wheel /Library/LaunchDaemons/com.lumi.gimateec.plist
  chmod 644 /Library/LaunchDaemons/com.lumi.gimateec.plist
  plutil -lint /Library/LaunchDaemons/com.lumi.gimateec.plist
  launchctl bootout system/com.lumi.gimateec 2>/dev/null || true
  launchctl bootstrap system /Library/LaunchDaemons/com.lumi.gimateec.plist
  echo "daemon cargado"

  cd /
  sync
  diskutil unmount "$M" >/dev/null
  echo DESMONTADO
} > "$OUT" 2>&1
chmod 644 "$OUT" || true
