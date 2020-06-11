#!/usr/bin/env python3
# OpenMPT packaging script by Saga Musix
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
openmpt_zip_32bit_basepath = "installer/OpenMPT-" + openmpt_version + "/"
openmpt_zip_64bit_basepath = "installer/OpenMPT-" + openmpt_version + "-x64/"
openmpt_zip_32bit_path = openmpt_zip_32bit_basepath + openmpt_version_name + "/"
openmpt_zip_64bit_path = openmpt_zip_64bit_basepath + openmpt_version_name + "/"

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
    copy_file("packageTemplate/", to_path, "OpenMPT Fie Icon.ico")
    copy_file("packageTemplate/", to_path, "Release Notes.html")
    copy_file("packageTemplate/", to_path, "OpenMPT Manual.chm")
    copy_file("packageTemplate/", to_path, "readme.txt")

remove_dir(openmpt_zip_32bit_basepath)
remove_dir(openmpt_zip_64bit_basepath)

remove_file("installer/" + openmpt_version_name + "-Setup.exe")
remove_file("installer/" + openmpt_version_name + "-Setup-x64.exe")
remove_file("installer/" + openmpt_version_name + ".zip")
remove_file("installer/" + openmpt_version_name + "-x64.zip")
remove_file("installer/" + openmpt_version_name + "-portable.zip")
remove_file("installer/" + openmpt_version_name + "-portable-x64.zip")
remove_file("installer/" + openmpt_version_name + "-Setup.exe.digests")
remove_file("installer/" + openmpt_version_name + "-Setup-x64.exe.digests")
remove_file("installer/" + openmpt_version_name + ".zip.digests")
remove_file("installer/" + openmpt_version_name + "-x64.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable.zip.digests")
remove_file("installer/" + openmpt_version_name + "-portable-x64.zip.digests")

print("Generating manual...")
pManual = Popen([executable, "wiki.py"], cwd="mptrack/manual_generator/")
if singleThreaded:
	pManual.communicate()
	if(pManual.returncode != 0):
			raise Exception("Something went wrong during manual creation!")

print("Copying 32-bit binaries...")
shutil.rmtree(openmpt_zip_32bit_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win7-static/x86/", openmpt_zip_32bit_path)
copy_pluginbridge("bin/release/vs2019-win7-static/", "x86", openmpt_zip_32bit_path)
copy_pluginbridge("bin/release/vs2019-win7-static/", "amd64", openmpt_zip_32bit_path)
Path(openmpt_zip_32bit_path + "OpenMPT.portable").touch()
print("Copying 64-bit binaries...")
shutil.rmtree(openmpt_zip_64bit_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2019-win7-static/amd64/", openmpt_zip_64bit_path)
copy_pluginbridge("bin/release//vs2019-win7-static/", "x86", openmpt_zip_64bit_path)
copy_pluginbridge("bin/release//vs2019-win7-static/", "amd64", openmpt_zip_64bit_path)
Path(openmpt_zip_64bit_path + "OpenMPT.portable").touch()

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
copy_other(openmpt_zip_32bit_path,    openmpt_version_short)
copy_other(openmpt_zip_64bit_path,    openmpt_version_short)

print("Creating zip files and installers...")
p7z32    = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable.zip",            openmpt_version_name + "/"], cwd=openmpt_zip_32bit_basepath)
if singleThreaded:
	p7z32.communicate()
p7z64    = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-portable-x64.zip",        openmpt_version_name + "/"], cwd=openmpt_zip_64bit_basepath)
if singleThreaded:
	p7z64.communicate()
pInno32  = Popen([pathISCC, "win32.iss"], cwd="installer/")
if singleThreaded:
	pInno32.communicate()
pInno64  = Popen([pathISCC, "win64.iss"], cwd="installer/")
if singleThreaded:
	pInno64.communicate()
if not singleThreaded:
	p7z32.communicate()
	p7z64.communicate()
	pInno32.communicate()
	pInno64.communicate()

if os.path.isfile('packageTemplate/ExampleSongs/nosongs.txt'):
	os.remove('packageTemplate/ExampleSongs/nosongs.txt')

if(p7z32.returncode != 0 or p7z64.returncode != 0 or pInno32.returncode != 0 or pInno64.returncode != 0):
    raise Exception("Something went wrong during packaging!")

def hash_file(filename):
    md5 = hashlib.md5()
    sha1 = hashlib.sha1()
    sha512 = hashlib.sha512()
    with open(filename, "rb") as f:
        buf = f.read()
        md5.update(buf)
        sha1.update(buf)
        sha512.update(buf)
    with open(filename + ".digests", "wb") as f:
        f.write(("MD5: " + md5.hexdigest() + "\nSHA-1: " + sha1.hexdigest() + "\nSHA-512: " + sha512.hexdigest()).encode('utf-8'))
        f.close()

hash_file("installer/" + openmpt_version_name + "-Setup.exe")
hash_file("installer/" + openmpt_version_name + "-Setup-x64.exe")
hash_file("installer/" + openmpt_version_name + "-portable.zip")
hash_file("installer/" + openmpt_version_name + "-portable-x64.zip")

shutil.rmtree(openmpt_zip_32bit_basepath)
shutil.rmtree(openmpt_zip_64bit_basepath)

if interactive:
	input(openmpt_version_name + " has been packaged successfully.")
