#!/usr/bin/env python3

import datetime
import json
import os

OPENMPT_VERSION_MAJORMAJOR = os.environ['OPENMPT_VERSION_MAJORMAJOR']
OPENMPT_VERSION_MAJOR = os.environ['OPENMPT_VERSION_MAJOR']
OPENMPT_VERSION_MINOR = os.environ['OPENMPT_VERSION_MINOR']
OPENMPT_VERSION_MINORMINOR = os.environ['OPENMPT_VERSION_MINORMINOR']
SVNVERSION = os.environ['SVNVERSION']

os.chdir(os.path.dirname(os.path.abspath(__file__)))
os.chdir("..")

version = OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR if OPENMPT_VERSION_MINORMINOR == "00" else OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-" + SVNVERSION

update = {
	"OpenMPT " + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR: {
		"version": version,
		"date": datetime.datetime.utcnow().isoformat(),
		"announcement_url": "https://builds.openmpt.org/",
		"changelog_url": "https://source.openmpt.org/browse/openmpt/trunk/OpenMPT/?op=log&isdir=1&",
		"downloads": {
			"installer": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-Setup.exe",
				"filename": "OpenMPT-" + version + "-Setup.exe",
				"is_installer": True,
				"is_archive": False,
				"autoupdate": True,
				"autoupdate_installer_arguments": [ "/SP-", "/SILENT", "/NOCANCEL", "/AUTOUPDATE=yes" ],
				"autoupdate_installer_required_architectures": { "x86":True },
				"autoupdate_archive_subfolder": "",
				"autoupdate_archive_restartbinary": "",
				"supported_architectures": { "x86":True,"amd64":True,"arm":True,"arm64":True },
				"required_processor_features": { "x86":{"sse2":True}, "amd64":{"sse2":True} },
				"required_os": { "version_major":6, "version_minor":1, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			},
			"portable-x86": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86.zip",
				"filename": "OpenMPT-" + version + "-portable-x86.zip",
				"is_installer": False,
				"is_archive": True,
				"autoupdate": True,
				"autoupdate_installer_arguments": [],
				"autoupdate_installer_required_architectures": {},
				"autoupdate_archive_subfolder": "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR,
				"autoupdate_archive_restartbinary": "OpenMPT.exe",
				"supported_architectures": { "x86":True },
				"required_processor_features": { "x86":{"sse2":True} },
				"required_os": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			},
			"portable-x86-legacy": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-x86-legacy.zip",
				"filename": "OpenMPT-" + version + "-portable-x86-legacy.zip",
				"is_installer": False,
				"is_archive": True,
				"autoupdate": True,
				"autoupdate_installer_arguments": [],
				"autoupdate_installer_required_architectures": {},
				"autoupdate_archive_subfolder": "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR,
				"autoupdate_archive_restartbinary": "OpenMPT.exe",
				"supported_architectures": { "x86":True },
				"required_processor_features": { "x86":{"sse2":True} },
				"required_os": { "version_major":6, "version_minor":1, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			},
			"portable-amd64": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64.zip",
				"filename": "OpenMPT-" + version + "-portable-amd64.zip",
				"is_installer": False,
				"is_archive": True,
				"autoupdate": True,
				"autoupdate_installer_arguments": [],
				"autoupdate_installer_required_architectures": {},
				"autoupdate_archive_subfolder": "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR,
				"autoupdate_archive_restartbinary": "OpenMPT.exe",
				"supported_architectures": { "amd64":True },
				"required_processor_features": { "amd64":{"sse2":True} },
				"required_os": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			},
			"portable-amd64-legacy": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-amd64-legacy.zip",
				"filename": "OpenMPT-" + version + "-portable-amd64-legacy.zip",
				"is_installer": False,
				"is_archive": True,
				"autoupdate": True,
				"autoupdate_installer_arguments": [],
				"autoupdate_installer_required_architectures": {},
				"autoupdate_archive_subfolder": "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR,
				"autoupdate_archive_restartbinary": "OpenMPT.exe",
				"supported_architectures": { "amd64":True },
				"required_processor_features": { "amd64":{"sse2":True} },
				"required_os": { "version_major":6, "version_minor":1, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			},
			"portable-arm": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm.zip",
				"filename": "OpenMPT-" + version + "-portable-arm.zip",
				"is_installer": False,
				"is_archive": True,
				"autoupdate": True,
				"autoupdate_installer_arguments": [],
				"autoupdate_installer_required_architectures": {},
				"autoupdate_archive_subfolder": "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR,
				"autoupdate_archive_restartbinary": "OpenMPT.exe",
				"supported_architectures": { "arm":True },
				"required_processor_features": { "arm":{} },
				"required_os": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			},
			"portable-arm64": {
				"url": "https://builds.openmpt.org/builds/auto/openmpt/pkg.win/" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "/OpenMPT-" + version + "-portable-arm64.zip",
				"filename": "OpenMPT-" + version + "-portable-arm64.zip",
				"is_installer": False,
				"is_archive": True,
				"autoupdate": True,
				"autoupdate_installer_arguments": [],
				"autoupdate_installer_required_architectures": {},
				"autoupdate_archive_subfolder": "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR,
				"autoupdate_archive_restartbinary": "OpenMPT.exe",
				"supported_architectures": { "arm64":True },
				"required_processor_features": { "arm64":{} },
				"required_os": { "version_major":10, "version_minor":0, "servicepack_major":0, "servicepack_minor":0, "build":0, "wine_major":1, "wine_minor":8, "wine_update":0 }
			}
		}
	}
}

with open("installer/" + "OpenMPT-" + OPENMPT_VERSION_MAJORMAJOR + "." + OPENMPT_VERSION_MAJOR + "." + OPENMPT_VERSION_MINOR + "." + OPENMPT_VERSION_MINORMINOR + "-update.json", "wb") as f:
	f.write((json.dumps(update, ensure_ascii=False, indent=1)).encode('utf-8'))
	f.close()
