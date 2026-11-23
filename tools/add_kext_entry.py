import plistlib, re, sys

# uso: add_kext2.py <BundlePath> <ExecutablePath> <Comment>
if len(sys.argv) < 4:
    print("uso: add_kext2.py Bundle.kext Contents/MacOS/Bin Comment")
    sys.exit(1)
bundle, exe, comment = sys.argv[1], sys.argv[2], sys.argv[3]

P = "/Volumes/NO NAME/EFI/OC/config.plist"
t = open(P, encoding="utf-8").read()
orig = t

if bundle in t:
    print("ya existe la entrada", bundle)
    sys.exit(0)

k = t.find("<key>Kernel</key>")
i = t.find("<key>Add</key>", k)
a = t.find("<array>", i) + len("<array>")
depth, j = 1, a
while depth:
    na, nc = t.find("<array>", j), t.find("</array>", j)
    if na != -1 and na < nc:
        depth += 1; j = na + len("<array>")
    else:
        depth -= 1; j = nc + len("</array>")
close = nc

fs = t.find("<dict>", a)
fe = t.find("</dict>", fs) + len("</dict>")
tpl = t[fs:fe]

def set_str(block, key, value):
    m = re.search(r"(<key>%s</key>\s*<string>)(.*?)(</string>)" % key, block, re.S)
    if not m:
        return block
    return block[:m.start(2)] + value + block[m.end(2):]

new = tpl
new = set_str(new, "BundlePath", bundle)
new = set_str(new, "Comment", comment)
new = set_str(new, "ExecutablePath", exe)
new = set_str(new, "MinKernel", "")
new = set_str(new, "MaxKernel", "")
new = new.replace("<false/>", "<true/>", 1)

indent = t[t.rfind("\n", 0, fs) + 1:fs]
new = "\n".join(indent + ln.lstrip("\t") if ln.strip() else ln for ln in new.split("\n"))
out = t[:close].rstrip() + "\n" + new + "\n\t\t" + t[close:]
open(P, "w", encoding="utf-8").write(out)

try:
    b = plistlib.load(open(P, "rb"))
except Exception as e:
    open(P, "w", encoding="utf-8").write(orig)
    print("error, restaurado:", e); sys.exit(1)

e = b["Kernel"]["Add"][-1]
print({x: e[x] for x in ("BundlePath", "ExecutablePath", "Enabled")})
