#!/usr/bin/env python3
# OpenMPT packaging script
# https://openmpt.org/

from pathlib import Path
from subprocess import Popen
from sys import executable
import os, shutil, hashlib
import sys

path7z = "C:\\Program Files\\7-Zip\\7z.exe"
pathISCC = "C:\\Program Files (x86)\\Inno Setup\\ISCC.exe"
singleThreaded = False
interactive = True

for arg in sys.argv:
	if arg == '--localtools':
		path7z = "build\\tools\\7zip\\7z.exe"
		pathISCC = "build\\tools\\innosetup\\{app}\\ISCC.exe"
	if arg == '--singlethreaded':
		singleThreaded = True
	if arg == '--noninteractive':
		interactive = False

def get_version_number():
    with open('common/versionNumber.h', 'r') as f:
        lines = f.readlines()
    majormajor = 0
    major = 0
    minor = 0
    minorminor = 0
    for line in lines:
        tokens = line.split()
        if len(tokens) == 3:
            if tokens[0] == '#define':
                if tokens[1] == 'VER_MAJORMAJOR':
                    majormajor = int(tokens[2])
                if tokens[1] == 'VER_MAJOR':
                    major = int(tokens[2])
                if tokens[1] == 'VER_MINOR':
                    minor = int(tokens[2])
                if tokens[1] == 'VER_MINORMINOR':
                    minorminor = int(tokens[2])
    if majormajor < 1:
        raise Exception("Could not parse version!")
    if major < 1:
        raise Exception("Could not parse version!")
    if minor == 0 and minorminor == 0:
        raise Exception("Could not parse version!")
    return ("%d.%02d.%02d.%02d" % (majormajor, major, minor, minorminor), "%d.%02d" % (majormajor, major))

os.chdir(os.path.dirname(os.path.abspath(__file__)))
os.chdir("../..")

(openmpt_version, openmpt_version_short) = get_version_number()

def remove_file(filename):
	if os.path.isfile(filename):
		os.remove(filename)

def remove_dir(dirname):
	if os.path.isdir(dirname):
		shutil.rmtree(dirname)

openmpt_version_name = "OpenMPT-" + openmpt_version
openmpt_zip_x86_basepath = "installer/OpenMPT-" + openmpt_version + "-x86/"
openmpt_zip_x86_legacy_basepath = "installer/OpenMPT-" + openmpt_version + "-x86-legacy/"
openmpt_zip_amd64_basepath = "installer/OpenMPT-" + openmpt_version + "-amd64/"
openmpt_zip_amd64_legacy_basepath = "installer/OpenMPT-" + openmpt_version + "-amd64-legacy/"
openmpt_zip_arm_basepath = "installer/OpenMPT-" + openmpt_version + "-arm/"
openmpt_zip_arm64_basepath = "installer/OpenMPT-" + openmpt_version + "-arm64/"
openmpt_zip_symbols_basepath = "installer/OpenMPT-" + openmpt_version + "-symbols/"
openmpt_zip_x86_path = openmpt_zip_x86_basepath
openmpt_zip_x86_legacy_path = openmpt_zip_x86_legacy_basepath
openmpt_zip_amd64_path = openmpt_zip_amd64_basepath
openmpt_zip_amd64_legacy_path = openmpt_zip_amd64_legacy_basepath
openmpt_zip_arm_path = openmpt_zip_arm_basepath
openmpt_zip_arm64_path = openmpt_zip_arm64_basepath
openmpt_zip_symbols_path = openmpt_zip_symbols_basepath

def copy_file(from_path, to_path, filename):
    shutil.copyfile(from_path + filename, to_path + filename)

def copy_tree(from_path, to_path, pathname):
    shutil.copytree(from_path + pathname, to_path + pathname)

def copy_binaries(from_path, to_path):
    os.makedirs(to_path)
    copy_file(from_path, to_path, "OpenMPT.exe")
    copy_file(from_path, to_path, "openmpt-lame.dll")
    copy_file(from_path, to_path, "openmpt-mpg123.dll")
    copy_file(from_path, to_path, "openmpt-soundtouch.dll")
    copy_file(from_path, to_path, "openmpt-wine-support.zip")

def copy_pluginbridge(from_path, arch, to_path):
    copy_file(from_path + arch + "/", to_path, "PluginBridge-" + arch + ".exe")
    copy_file(from_path + arch + "/", to_path, "PluginBridgeLegacy-" + arch + ".exe")

def copy_symbols(from_path, to_path):
    os.makedirs(to_path)
    copy_file(from_path, to_path, "OpenMPT.pdb")
    copy_file(from_path, to_path, "openmpt-lame.pdb")
    copy_file(from_path, to_path, "openmpt-mpg123.pdb")
    copy_file(from_path, to_path, "openmpt-soundtouch.pdb")

def copy_symbols_pluginbridge(from_path, to_path, arch):
    copy_file(from_path, to_path, "PluginBridge-" + arch + ".pdb")
    copy_file(from_path, to_path, "PluginBridgeLegacy-" + arch + ".pdb")

def copy_other(to_path, openmpt_version_short):
    copy_tree("packageTemplate/", to_path, "ExampleSongs")
    copy_tree("packageTemplate/", to_path, "extraKeymaps")
    copy_tree("packageTemplate/", to_path, "ReleaseNotesImages")
    copy_tree("packageTemplate/", to_path, "Licenses")
    copy_file("packageTemplate/", to_path, "History.txt")
    copy_file("packageTemplate/", to_path, "License.txt")
    copy_file("packageTemplate/", to_path, "OpenMPT Issue Tracker.url")
    copy_file("packageTemplate/", to_path, "OpenMPT Support and Community Forum.url")
    copy_file("packageTemplate/", to_path, "OpenMPT File Icon.ico")
    copy_file("packageTemplate/", to_path, "Release Notes.html")
    copy_file("packageTemplate/", to_path, "OpenMPT Manual.chm")
    copy_file("packageTemplate/", to_path, "readme.txt")

remove_dir(openmpt_zip_x86_basepath)
remove_dir(openmpt_zip_amd64_basepath)
remove_dir(openmpt_zip_arm_basepath)
remove_dir(openmpt_zip_arm64_basepath)
remove_dir(openmpt_zip_symbols_basepath)

remove_file("installer/" + openmpt_version_name + "-Setup.exe")
remove_file("installer/" + openmpt_version_name + "-Setup.update.json")
remove_file("installer/" + openmpt_version_name + "-portable-x86.zip")
remove_file("installer/" + openmpt_version_name + "-portable-x86.update.json")
remove_file("installer/" + openmpt_version_name + "-portable-x86-legacy.zip")
remove_file("installer/" + openmpt_version_name + "-portable-x86-legacy.update.json")
remove_file("installer/" + openmpt_version_name + "-portable-amd64.zip")
remove_file("installer/" + openmpt_version_name + "-portable-amd64.update.json")
remove_file("installer/" + openmpt_version_name + "-portable-amd64-legacy.zip")
remove_file("installer/" + openmpt_version_name + "-portable-amd64-legacy.update.json")
remove_file("installer/" + openmpt_version_name + "-portable-arm.zip")
remove_file("installer/" + openmpt_version_name + "-portable-arm.update.json")
remove_file("installer/" + openmpt_version_name + "-portable-arm64.zip")
remove_file("installer/" + openmpt_version_name + "-portable-arm64.update.json")
remove_file("installer/" + openmpt_version_name + "-symbols.7z")
remove_file("installer/" + openmpt_version_name + "-Setup.exe.digests")
remove_file("installer/" + openmpt_version_name + "-portable-x86.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-x86-legacy.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-amd64.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-amd64-legacy.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-arm.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-arm64.zip.digests")
remove_file("installer/" + openmpt_version_name + "-symbols.7z.digests")
remove_file("installer/" + openmpt_version_name + "-update.json")

print("Generating manual...")
pManual = Popen([executable, "wiki.py"], cwd="mptrack/manual_generator/")
if singleThreaded:
	pManual.communicate()
	if(pManual.returncode != 0):
		raise Exception("Something went wrong during manual creation!")

print("Copying x86 binaries...")
shutil.rmtree(openmpt_zip_x86_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win10-static/x86/", openmpt_zip_x86_path)
copy_pluginbridge("bin/release/vs2019-win10-static/", "x86", openmpt_zip_x86_path)
copy_pluginbridge("bin/release/vs2019-win10-static/", "amd64", openmpt_zip_x86_path)
copy_pluginbridge("bin/release/vs2019-win10-static/", "arm", openmpt_zip_x86_path)
copy_pluginbridge("bin/release/vs2019-win10-static/", "arm64", openmpt_zip_x86_path)
Path(openmpt_zip_x86_path + "OpenMPT.portable").touch()
print("Copying x86 legacy binaries...")
shutil.rmtree(openmpt_zip_x86_legacy_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win7-static/x86/", openmpt_zip_x86_legacy_path)
copy_pluginbridge("bin/release/vs2019-win7-static/", "x86", openmpt_zip_x86_legacy_path)
copy_pluginbridge("bin/release/vs2019-win7-static/", "amd64", openmpt_zip_x86_legacy_path)
Path(openmpt_zip_x86_legacy_path + "OpenMPT.portable").touch()
print("Copying amd64 binaries...")
shutil.rmtree(openmpt_zip_amd64_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win10-static/amd64/", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "x86", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "amd64", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm64", openmpt_zip_amd64_path)
Path(openmpt_zip_amd64_path + "OpenMPT.portable").touch()
print("Copying amd64 legacy binaries...")
shutil.rmtree(openmpt_zip_amd64_legacy_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win7-static/amd64/", openmpt_zip_amd64_legacy_path)
copy_pluginbridge("bin/release//vs2019-win7-static/", "x86", openmpt_zip_amd64_legacy_path)
copy_pluginbridge("bin/release//vs2019-win7-static/", "amd64", openmpt_zip_amd64_legacy_path)
Path(openmpt_zip_amd64_legacy_path + "OpenMPT.portable").touch()
print("Copying arm binaries...")
shutil.rmtree(openmpt_zip_arm_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win10-static/arm/", openmpt_zip_arm_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "x86", openmpt_zip_arm_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "amd64", openmpt_zip_arm_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm", openmpt_zip_arm_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm64", openmpt_zip_arm_path)
Path(openmpt_zip_arm_path + "OpenMPT.portable").touch()
print("Copying arm64 binaries...")
shutil.rmtree(openmpt_zip_arm64_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win10-static/arm64/", openmpt_zip_arm64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "x86", openmpt_zip_arm64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "amd64", openmpt_zip_arm64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm", openmpt_zip_arm64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm64", openmpt_zip_arm64_path)
Path(openmpt_zip_arm64_path + "OpenMPT.portable").touch()

print("Copying symbols...")
shutil.rmtree(openmpt_zip_symbols_basepath, ignore_errors=True)
copy_symbols("bin/release/vs2019-win10-static/" + "x86/", openmpt_zip_symbols_path  + "x86/")
copy_symbols("bin/release/vs2019-win7-static/" + "x86/", openmpt_zip_symbols_path  + "x86-legacy/")
copy_symbols("bin/release/vs2019-win10-static/" + "amd64/", openmpt_zip_symbols_path  + "amd64/")
copy_symbols("bin/release/vs2019-win7-static/" + "amd64/", openmpt_zip_symbols_path  + "amd64-legacy/")
copy_symbols("bin/release/vs2019-win10-static/" + "arm/", openmpt_zip_symbols_path  + "arm/")
copy_symbols("bin/release/vs2019-win10-static/" + "arm64/", openmpt_zip_symbols_path  + "arm64/")
copy_symbols_pluginbridge("bin/release/vs2019-win10-static/" + "x86/", openmpt_zip_symbols_path  + "x86/", "x86")
copy_symbols_pluginbridge("bin/release/vs2019-win7-static/" + "x86/", openmpt_zip_symbols_path  + "x86-legacy/", "x86")
copy_symbols_pluginbridge("bin/release/vs2019-win10-static/" + "amd64/", openmpt_zip_symbols_path  + "amd64/", "amd64")
copy_symbols_pluginbridge("bin/release/vs2019-win7-static/" + "amd64/", openmpt_zip_symbols_path  + "amd64-legacy/", "amd64")
copy_symbols_pluginbridge("bin/release/vs2019-win10-static/" + "arm/", openmpt_zip_symbols_path  + "arm/", "arm")
copy_symbols_pluginbridge("bin/release/vs2019-win10-static/" + "arm64/", openmpt_zip_symbols_path  + "arm64/", "arm64")

if not singleThreaded:
	pManual.communicate()
	if(pManual.returncode != 0):
		raise Exception("Something went wrong during manual creation!")

print("Updating package template...")
pTemplate = Popen(["build\\auto\\update_package_template.cmd"], cwd="./")
pTemplate.communicate()
if(pTemplate.returncode != 0):
    raise Exception("Something went wrong during updating package template!")

print("Other package contents...")
copy_other(openmpt_zip_x86_path, openmpt_version_short)
copy_other(openmpt_zip_x86_legacy_path, openmpt_version_short)
copy_other(openmpt_zip_amd64_path, openmpt_version_short)
copy_other(openmpt_zip_amd64_legacy_path, openmpt_version_short)
copy_other(openmpt_zip_arm_path, openmpt_version_short)
copy_other(openmpt_zip_arm64_path, openmpt_version_short)

print("Creating zip files and installers...")

pInno = Popen([pathISCC, "install-multi-arch.iss"], cwd="installer/")
if singleThreaded:
	pInno.communicate()

p7zx86 = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-x86.zip", "."], cwd=openmpt_zip_x86_basepath)
if singleThreaded:
	p7zx86.communicate()
p7zx86legacy = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-x86-legacy.zip", "."], cwd=openmpt_zip_x86_legacy_basepath)
if singleThreaded:
	p7zx86legacy.communicate()
p7zamd64 = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-amd64.zip", "."], cwd=openmpt_zip_amd64_basepath)
if singleThreaded:
	p7zamd64.communicate()
p7zamd64legacy = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-amd64-legacy.zip", "."], cwd=openmpt_zip_amd64_legacy_basepath)
if singleThreaded:
	p7zamd64legacy.communicate()
p7zarm = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-arm.zip", "."], cwd=openmpt_zip_arm_basepath)
if singleThreaded:
	p7zarm.communicate()
p7zarm64 = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-arm64.zip", "."], cwd=openmpt_zip_arm64_basepath)
if singleThreaded:
	p7zarm64.communicate()

p7zsymbols = Popen([path7z, "a", "-t7z", "-mx=9", "../" + openmpt_version_name + "-symbols.7z", "."], cwd=openmpt_zip_symbols_basepath)
if singleThreaded:
	p7zsymbols.communicate()

if not singleThreaded:
	pInno.communicate()
	p7zx86.communicate()
	p7zx86legacy.communicate()
	p7zamd64.communicate()
	p7zamd64legacy.communicate()
	p7zarm.communicate()
	p7zarm64.communicate()
	p7zsymbols.communicate()

if(p7zx86.returncode != 0 or p7zx86legacy.returncode != 0 or p7zamd64.returncode != 0 or p7zamd64legacy.returncode != 0 or p7zarm.returncode != 0 or p7zarm64.returncode != 0 or pInno.returncode != 0):
    raise Exception("Something went wrong during packaging!")

def hash_file(filename):
    sha3_512 = hashlib.sha3_512()
    with open(filename, "rb") as f:
        buf = f.read()
        sha3_512.update(buf)
    with open(filename + ".digests", "wb") as f:
        f.write(("SHA3-512: " + sha3_512.hexdigest() + "\n").encode('utf-8'))
        f.close()

hash_file("installer/" + openmpt_version_name + "-Setup.exe")
hash_file("installer/" + openmpt_version_name + "-portable-x86.zip")
hash_file("installer/" + openmpt_version_name + "-portable-x86-legacy.zip")
hash_file("installer/" + openmpt_version_name + "-portable-amd64.zip")
hash_file("installer/" + openmpt_version_name + "-portable-amd64-legacy.zip")
hash_file("installer/" + openmpt_version_name + "-portable-arm.zip")
hash_file("installer/" + openmpt_version_name + "-portable-arm64.zip")
hash_file("installer/" + openmpt_version_name + "-symbols.7z")

shutil.rmtree(openmpt_zip_x86_basepath)
shutil.rmtree(openmpt_zip_x86_legacy_basepath)
shutil.rmtree(openmpt_zip_amd64_basepath)
shutil.rmtree(openmpt_zip_amd64_legacy_basepath)
shutil.rmtree(openmpt_zip_arm_basepath)
shutil.rmtree(openmpt_zip_arm64_basepath)
shutil.rmtree(openmpt_zip_symbols_basepath)

if interactive:
	input(openmpt_version_name + " has been packaged successfully.")
