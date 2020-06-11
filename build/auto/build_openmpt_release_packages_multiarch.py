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
exampleSongs = True

for arg in sys.argv:
	if arg == '--localtools':
		path7z = "build\\tools\\7zip\\7z.exe"
		pathISCC = "build\\tools\\innosetup\\{app}\\ISCC.exe"
	if arg == '--singlethreaded':
		singleThreaded = True
	if arg == '--noninteractive':
		interactive = False
	if arg == '--noexamplesongs':
		exampleSongs = False

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
openmpt_zip_amd64_basepath = "installer/OpenMPT-" + openmpt_version + "-amd64/"
openmpt_zip_arm_basepath = "installer/OpenMPT-" + openmpt_version + "-arm/"
openmpt_zip_arm64_basepath = "installer/OpenMPT-" + openmpt_version + "-arm64/"
openmpt_zip_x86_path = openmpt_zip_x86_basepath + openmpt_version_name + "/"
openmpt_zip_amd64_path = openmpt_zip_amd64_basepath + openmpt_version_name + "/"
openmpt_zip_arm_path = openmpt_zip_arm_basepath + openmpt_version_name + "/"
openmpt_zip_arm64_path = openmpt_zip_arm64_basepath + openmpt_version_name + "/"

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

remove_file("installer/" + openmpt_version_name + "-Setup.exe")
remove_file("installer/" + openmpt_version_name + "-portable-x86.zip")
remove_file("installer/" + openmpt_version_name + "-portable-amd64.zip")
remove_file("installer/" + openmpt_version_name + "-portable-arm.zip")
remove_file("installer/" + openmpt_version_name + "-portable-arm64.zip")
remove_file("installer/" + openmpt_version_name + "-Setup.exe.digests")
remove_file("installer/" + openmpt_version_name + "-portable-x86.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-amd64.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-arm.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-arm64.zip.digests")

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
print("Copying amd64 binaries...")
shutil.rmtree(openmpt_zip_amd64_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win10-static/amd64/", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "x86", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "amd64", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm", openmpt_zip_amd64_path)
copy_pluginbridge("bin/release//vs2019-win10-static/", "arm64", openmpt_zip_amd64_path)
Path(openmpt_zip_amd64_path + "OpenMPT.portable").touch()
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

if not singleThreaded:
	pManual.communicate()
	if(pManual.returncode != 0):
			raise Exception("Something went wrong during manual creation!")

if os.path.isfile('packageTemplate/ExampleSongs/nosongs.txt'):
	os.remove('packageTemplate/ExampleSongs/nosongs.txt')
if not exampleSongs:
	os.makedirs('packageTemplate/ExampleSongs', exist_ok=True)
	with open('packageTemplate/ExampleSongs/nosongs.txt', 'w') as f:
		f.write('Installer for OpenMPT test versions contains no example songs.')
		f.write('\n')

print("Updating package template...")
pTemplate = Popen(["build\\auto\\update_package_template.cmd"], cwd="./")
pTemplate.communicate()
if(pTemplate.returncode != 0):
    raise Exception("Something went wrong during updating package template!")

print("Other package contents...")
copy_other(openmpt_zip_x86_path, openmpt_version_short)
copy_other(openmpt_zip_amd64_path, openmpt_version_short)
copy_other(openmpt_zip_arm_path, openmpt_version_short)
copy_other(openmpt_zip_arm64_path, openmpt_version_short)

print("Creating zip files and installers...")

pInno = Popen([pathISCC, "install-multi-arch.iss"], cwd="installer/")
if singleThreaded:
	pInno.communicate()

p7zx86 = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-x86.zip", openmpt_version_name + "/"], cwd=openmpt_zip_x86_basepath)
if singleThreaded:
	p7zx86.communicate()
p7zamd64 = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-amd64.zip", openmpt_version_name + "/"], cwd=openmpt_zip_amd64_basepath)
if singleThreaded:
	p7zamd64.communicate()
p7zarm = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-arm.zip", openmpt_version_name + "/"], cwd=openmpt_zip_arm_basepath)
if singleThreaded:
	p7zarm.communicate()
p7zarm64 = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-arm64.zip", openmpt_version_name + "/"], cwd=openmpt_zip_arm64_basepath)
if singleThreaded:
	p7zarm64.communicate()

if not singleThreaded:
	pInno.communicate()
	p7zx86.communicate()
	p7zamd64.communicate()
	p7zarm.communicate()
	p7zarm64.communicate()

if os.path.isfile('packageTemplate/ExampleSongs/nosongs.txt'):
	os.remove('packageTemplate/ExampleSongs/nosongs.txt')

if(p7zx86.returncode != 0 or p7zamd64.returncode != 0 or p7zarm.returncode != 0 or p7zarm64.returncode != 0 or pInno.returncode != 0):
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
hash_file("installer/" + openmpt_version_name + "-portable-amd64.zip")
hash_file("installer/" + openmpt_version_name + "-portable-arm.zip")
hash_file("installer/" + openmpt_version_name + "-portable-arm64.zip")

shutil.rmtree(openmpt_zip_x86_basepath)
shutil.rmtree(openmpt_zip_amd64_basepath)
shutil.rmtree(openmpt_zip_arm_basepath)
shutil.rmtree(openmpt_zip_arm64_basepath)

if interactive:
	input(openmpt_version_name + " has been packaged successfully.")
