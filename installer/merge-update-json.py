#!/usr/bin/env python3

import datetime
import hashlib
import json
import os
import sys

OpenMPTVer = sys.argv[1]
outfile = sys.argv[2]
dstfile = sys.argv[3]
srcfile = sys.argv[4]

with open(dstfile, "rb") as f:
	dst = json.load(f)
	f.close()

with open(srcfile, "rb") as f:
	src = json.load(f)
	f.close()

if dst[OpenMPTVer]["version"] != src[OpenMPTVer]["version"]:
	print("Skipping update JSON merge for versions " + dst[OpenMPTVer]["version"] + " and " + src[OpenMPTVer]["version"])
	sys.exit(0)

for download, data in src[OpenMPTVer]["downloads"].items():
	dst[OpenMPTVer]["downloads"][download] = data

with open(outfile, "wb") as f:
	f.write((json.dumps(dst, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

print("Merged update JSON for version " + src[OpenMPTVer]["version"])
sys.exit(0)
