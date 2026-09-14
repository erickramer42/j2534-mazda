import glob, os
d = open(max(glob.glob(r"test\traces\*.log"), key=os.path.getmtime), "rb").read()
i = d.find(b"\x00")
print("NUL count:", d.count(b"\x00"), "at offset", i)
print(repr(d[max(0,i-150):i+60]))   # what precedes/follows the run
print([l[:60] for l in d.splitlines() if b"\x00" in l][:3])