#!/usr/bin/env python3

import datetime
import hashlib
import json
import os

from subprocess import Popen

OPENMPT_VERSION_MAJORMAJOR = os.environ['OPENMPT_VERSION_MAJORMAJOR']
OPENMPT_VERSION_MAJOR = os.environ['OPENMPT_VERSION_MAJOR']
OPENMPT_VERSION_MINOR = os.environ['OPENMPT_VERSION_MINOR']
OPENMPT_VERSION_MINORMINOR = os.environ['OPENMPT_VERSION_MINORMINOR']
SVNVERSION = os.environ['SVNVERSION']

os.chdir(os.path.dirname(os.path.abspath(__file__)))
os.chdir("..")

plainversion = OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR
version = OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR if OPENMPT_VERSION_MINORMINOR == "00" else OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-" + SVNVERSION

def hash_file_sha512(filename):
	sha512 = hashlib.sha512()
	with open(filename, "rb") as f:
		sha512.update(f.read())
	return sha512.hexdigest()
def hash_file_sha3_512(filename):
	sha3_512 = hashlib.sha3_512()
	with open(filename, "rb") as f:
		sha3_512.update(f.read())
	return sha3_512.hexdigest()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-Setup.exe",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-Setup.exe"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-Setup.exe"),
	},
	"filename": "OpenMPT-" + version + "-Setup.exe",
	"autoupdate_installer": {
		"arguments": [ "/SP-", "/SILENT", "/NOCANCEL", "/AUTOUPDATE=yes" ]
	},
	"autoupdate_archive": None
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-Setup.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86.zip",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-portable-x86.zip"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-portable-x86.zip"),
	},
	"filename": "OpenMPT-" + version + "-portable-x86.zip",
	"autoupdate_installer": None,
	"autoupdate_archive": {
		"subfolder": "",
		"restartbinary": "OpenMPT.exe"
	}
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-x86.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86-legacy.zip",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-portable-x86-legacy.zip"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-portable-x86-legacy.zip"),
	},
	"filename": "OpenMPT-" + version + "-portable-x86-legacy.zip",
	"autoupdate_installer": None,
	"autoupdate_archive": {
		"subfolder": "",
		"restartbinary": "OpenMPT.exe"
	}
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-x86-legacy.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64.zip",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-portable-amd64.zip"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-portable-amd64.zip"),
	},
	"filename": "OpenMPT-" + version + "-portable-amd64.zip",
	"autoupdate_installer": None,
	"autoupdate_archive": {
		"subfolder": "",
		"restartbinary": "OpenMPT.exe"
	}
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-amd64.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64-legacy.zip",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-portable-amd64-legacy.zip"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-portable-amd64-legacy.zip"),
	},
	"filename": "OpenMPT-" + version + "-portable-amd64-legacy.zip",
	"autoupdate_installer": None,
	"autoupdate_archive": {
		"subfolder": "",
		"restartbinary": "OpenMPT.exe"
	}
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-amd64-legacy.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm.zip",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-portable-arm.zip"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-portable-arm.zip"),
	},
	"filename": "OpenMPT-" + version + "-portable-arm.zip",
	"autoupdate_installer": None,
	"autoupdate_archive": {
		"subfolder": "",
		"restartbinary": "OpenMPT.exe"
	}
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-arm.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

update = {
	"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm64.zip",
	"checksums": {
		"SHA-512": hash_file_sha512("installer/OpenMPT-" + plainversion + "-portable-arm64.zip"),
		"SHA3-512": hash_file_sha3_512("installer/OpenMPT-" + plainversion + "-portable-arm64.zip"),
	},
	"filename": "OpenMPT-" + version + "-portable-arm64.zip",
	"autoupdate_installer": None,
	"autoupdate_archive": {
		"subfolder": "",
		"restartbinary": "OpenMPT.exe"
	}
}
with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-arm64.update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()



update = {
	"OpenMPT " + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR: {
		"version": version,
		"date": datetime.datetime.utcnow().isoformat(),
		"announcement_url": "https://builds.openmpt.org/",
		"changelog_url": "https://source.openmpt.org/browse/openmpt/trunk/OpenMPT/?op=log&isdir=1&",
		"downloads": {
			"installer": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-Setup.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-Setup.exe",
				"type": "installer",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":6, "version_minor":1, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": { "x86":True },
				"supported_architectures": { "x86":True,"amd64":True,"arm":True,"arm64":True },
				"required_processor_features": { "x86":{"sse2":True}, "amd64":{"sse2":True} }
			},
			"portable-x86": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86.zip",
				"type": "archive",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": {},
				"supported_architectures": { "x86":True },
				"required_processor_features": { "x86":{"sse2":True} }
			},
			"portable-x86-legacy": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86-legacy.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86-legacy.zip",
				"type": "archive",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":6, "version_minor":1, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": {},
				"supported_architectures": { "x86":True },
				"required_processor_features": { "x86":{"sse2":True} }
			},
			"portable-amd64": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64.zip",
				"type": "archive",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": {},
				"supported_architectures": { "amd64":True },
				"required_processor_features": { "amd64":{"sse2":True} }
			},
			"portable-amd64-legacy": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64-legacy.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64-legacy.zip",
				"type": "archive",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":6, "version_minor":1, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": {},
				"supported_architectures": { "amd64":True },
				"required_processor_features": { "amd64":{"sse2":True} }
			},
			"portable-arm": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm.zip",
				"type": "archive",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": {},
				"supported_architectures": { "arm":True },
				"required_processor_features": { "arm":{} }
			},
			"portable-arm64": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm64.update.json",
				"download_url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm64.zip",
				"type": "archive",
				"can_autoupdate": True,
				"autoupdate_minversion": "1.30.00.08",
				"os": "windows",
				"required_windows_version": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 },
				"required_architectures": {},
				"supported_architectures": { "arm64":True },
				"required_processor_features": { "arm64":{} }
			}
		}
	}
}

with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()

def sign_file(filename):
	p = Popen(["bin/release/vs2019-win7-static/amd64/signtool.exe", "sign", "jws", "auto", filename, filename + ".jws.json"])
	p.communicate()

sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-Setup.update.json")
sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-x86.update.json")
sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-x86-legacy.update.json")
sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-amd64.update.json")
sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-amd64-legacy.update.json")
sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-arm.update.json")
sign_file("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-portable-arm64.update.json")

pdumpkey = Popen(["bin/release/vs2019-win7-static/amd64/signtool.exe", "dumpkey", "auto", "installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-update-publickey.jwk.json"])
pdumpkey.communicate()
