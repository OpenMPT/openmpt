#!/usr/bin/env python3
# OpenMPT packaging script by Saga Musix
# http://openmpt.org/
# Requires pywin32 (https://sourceforge.net/projects/pywin32/)

from win32api import GetFileVersionInfo
from subprocess import Popen
from sys import executable
import os, shutil, hashlib

path7z = "C:\\Program Files\\7-Zip\\7z.exe"
pathISCC = "C:\\Program Files (x86)\\Inno Setup\\ISCC.exe"

def get_version_number(filename):
    info = GetFileVersionInfo (filename, "\\")
    ms = info['FileVersionMS']
    ls = info['FileVersionLS']
    return ("%d.%02d.%02d.%02d" % (ms / 65536, ms % 65536, ls / 65536, ls % 65536), "%d.%02d" % (ms / 65536, ms % 65536))

os.chdir(os.path.dirname(os.path.abspath(__file__)))
os.chdir("../..")

(openmpt_version, openmpt_version_short) =  get_version_number("bin/release/vs2010-static/x86-32-win7/mptrack.exe")

openmpt_version_name = "OpenMPT-" + openmpt_version
openmpt_zip_32bit_basepath = "installer/OpenMPT-" + openmpt_version + "/"
openmpt_zip_32bitold_basepath = "installer/OpenMPT-" + openmpt_version + "-legacy/"
openmpt_zip_64bit_basepath = "installer/OpenMPT-" + openmpt_version + "-x64/"
openmpt_zip_32bit_path = openmpt_zip_32bit_basepath + openmpt_version_name + "/"
openmpt_zip_32bitold_path = openmpt_zip_32bitold_basepath + openmpt_version_name + "/"
openmpt_zip_64bit_path = openmpt_zip_64bit_basepath + openmpt_version_name + "/"

def copy_file(from_path, to_path, filename):
    shutil.copyfile(from_path + filename, to_path + filename)

def copy_tree(from_path, to_path, pathname):
    shutil.copytree(from_path + pathname, to_path + pathname)

def copy_binaries(from_path, to_path):
    os.makedirs(to_path)
    #os.makedirs(to_path + "Plugins/MIDI")
    copy_file(from_path, to_path, "mptrack.exe")
    copy_file(from_path, to_path, "PluginBridge32.exe")
    copy_file(from_path, to_path, "PluginBridge64.exe")
    copy_file(from_path, to_path, "OpenMPT_SoundTouch_f32.dll")
    copy_file(from_path, to_path, "unmo3.dll")
    #copy_file(from_path, to_path + "Plugins/MIDI/", "MIDI Input Output.dll")

def copy_other(to_path, openmpt_version_short):
    copy_tree("packageTemplate/", to_path, "ExampleSongs")
    copy_tree("packageTemplate/", to_path, "extraKeymaps")
    copy_tree("packageTemplate/", to_path, "ReleaseNotesImages/general")
    copy_tree("packageTemplate/", to_path, "ReleaseNotesImages/" + openmpt_version_short)
    copy_tree("packageTemplate/", to_path, "Licenses")
    copy_file("packageTemplate/", to_path, "History.txt")
    copy_file("packageTemplate/", to_path, "License.txt")
    copy_file("packageTemplate/", to_path, "ModPlug Central.url")
    copy_file("packageTemplate/", to_path, "mpt.ico")
    copy_file("packageTemplate/", to_path, "OMPT_" + openmpt_version_short + "_ReleaseNotes.html")
    copy_file("packageTemplate/", to_path, "open_settings_folder.bat")
    copy_file("packageTemplate/", to_path, "OpenMPT Manual.chm")
    copy_file("packageTemplate/", to_path, "readme.txt")

print("Generating manual...")
pManual = Popen([executable, "wiki.py"], cwd="mptrack/manual_generator/")

print("Copying 32-bit binaries...")
shutil.rmtree(openmpt_zip_32bit_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2010-static/x86-32-win7/", openmpt_zip_32bit_path)
print("Copying 32-bit legacy binaries...")
shutil.rmtree(openmpt_zip_32bitold_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2008-static/x86-32-winxp/", openmpt_zip_32bitold_path)
print("Copying 64-bit binaries...")
shutil.rmtree(openmpt_zip_64bit_basepath, ignore_errors=True)
copy_binaries("bin/release/vs2010-static/x86-64-win7/", openmpt_zip_64bit_path)

pManual.communicate()
if(pManual.returncode != 0):
    raise Exception("Something went wrong during manual creation!")

print("Other package contents...")
copy_other(openmpt_zip_32bit_path,    openmpt_version_short)
copy_other(openmpt_zip_32bitold_path, openmpt_version_short)
copy_other(openmpt_zip_64bit_path,    openmpt_version_short)

print("Creating zip files and installers...")
p7z32    = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + ".zip",        openmpt_version_name + "/"], cwd=openmpt_zip_32bit_basepath)
p7z32old = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-legacy.zip", openmpt_version_name + "/"], cwd=openmpt_zip_32bitold_basepath)
p7z64    = Popen([path7z, "a", "-tzip", "-mx=9", "../" + openmpt_version_name + "-x64.zip",    openmpt_version_name + "/"], cwd=openmpt_zip_64bit_basepath)
pInno32  = Popen([pathISCC, "win32.iss"], cwd="installer/")
pInno64  = Popen([pathISCC, "win64.iss"], cwd="installer/")
p7z32.communicate()
p7z32old.communicate()
p7z64.communicate()
pInno32.communicate()
pInno64.communicate()

if(p7z32.returncode != 0 or p7z32old.returncode != 0 or p7z64.returncode != 0 or pInno32.returncode != 0 or pInno64.returncode != 0):
    raise Exception("Something went wrong during packaging!")

def hash_file(filename):
    md5 = hashlib.md5()
    sha1 = hashlib.sha1()
    with open(filename, "rb") as f:
        buf = f.read()
        md5.update(buf)
        sha1.update(buf)
    with open(filename + ".digests", "wb") as f:
        f.write(("MD5: " + md5.hexdigest() + "\nSHA-1: " + sha1.hexdigest()).encode('utf-8'))
        f.close()

hash_file("installer/" + openmpt_version_name + "-Setup.exe")
hash_file("installer/" + openmpt_version_name + "-Setup-x64.exe")
hash_file("installer/" + openmpt_version_name + ".zip")
hash_file("installer/" + openmpt_version_name + "-legacy.zip")
hash_file("installer/" + openmpt_version_name + "-x64.zip")

shutil.rmtree(openmpt_zip_32bit_basepath)
shutil.rmtree(openmpt_zip_32bitold_basepath)
shutil.rmtree(openmpt_zip_64bit_basepath)

input(openmpt_version_name + " has been packaged successfully.")
