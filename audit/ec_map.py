import re, sys

DSL = "/Users/Shared/efi-mux/DSDT.dsl"
txt = open(DSL, encoding="utf-8", errors="replace").read().splitlines()

# localizar Field (PECM, ...)
start = None
for i, l in enumerate(txt):
    if re.search(r'Field \(PECM', l):
        start = i
        break
if start is None:
    print("no encontré Field(PECM)"); sys.exit(1)

# base de la región
base = None
for l in txt:
    m = re.search(r'OperationRegion \(PECM, SystemMemory, (0x[0-9A-Fa-f]+)', l)
    if m:
        base = int(m.group(1), 16); break
print(f"PECM base = 0x{base:X}")

# recorrer el bloque
i = start + 1
while i < len(txt) and "{" not in txt[i]:
    i += 1
i += 1
bit = 0
out = []
depth = 1
while i < len(txt):
    l = txt[i].strip()
    if l.startswith("}"):
        break
    m = re.match(r'([A-Z_][A-Z0-9_]{3}),\s+(\d+),', l)
    m2 = re.match(r'Offset \((0x[0-9A-Fa-f]+)\)', l)
    m3 = re.match(r',\s+(\d+),', l)
    if m3:
        bit += int(m3.group(1))
    elif m:
        name, width = m.group(1), int(m.group(2))
        out.append((name, bit, width))
        bit += width
    elif m2:
        bit = int(m2.group(1), 16) * 8
    i += 1

want = {"RPM1","RPM2","CTMP","CTML","FAN1","FAN2","FLVL","KBLL","BCPS","BCPC","PLED","BLED",
        "TFAN","GFAN","CRAF","FANB","PFMM","XTAL","PSON","ADAP","DNLV"}
print(f"{'campo':6s} {'byte':>6s} {'bit':>4s} {'ancho':>5s}  dirección absoluta")
for name, b, w in out:
    if name in want:
        addr = base + b // 8
        print(f"{name:6s} 0x{b//8:02X}   {b%8:>3d}  {w:>4d}   0x{addr:X}")

print("\n--- todos los campos con 'FAN' o 'TEMP' o 'LED' ---")
for name, b, w in out:
    if re.search(r'FAN|TMP|TEMP|LED|RPM', name):
        print(f"{name:6s} byte 0x{b//8:02X} bit {b%8} ancho {w}")
