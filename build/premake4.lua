
solution "all"
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
 platforms { "x32", "x64" }

 dofile "../include/flac.premake4.lua"
 dofile "../include/lhasa.premake4.lua"
 dofile "../include/miniz.premake4.lua"
 dofile "../include/minizip.premake4.lua"
 dofile "../include/portaudio.premake4.lua"
 dofile "../include/portmidi.premake4.lua"
 dofile "../include/pugixml.premake4.lua"
 dofile "../include/r8brain.premake4.lua"
 dofile "../include/smbPitchShift.premake4.lua"
 dofile "../include/soundtouch.premake4.lua"
 dofile "../include/UnRAR.premake4.lua"
 dofile "../include/zlib.premake4.lua"
