
solution "all"
 configurations { "Debug", "Release", "ReleaseNoLTCG" }
 platforms { "x32", "x64" }

 dofile "../build/premake4-win/ext-flac.premake4.lua"
 dofile "../build/premake4-win/ext-lhasa.premake4.lua"
 dofile "../build/premake4-win/ext-miniz.premake4.lua"
 dofile "../build/premake4-win/ext-minizip.premake4.lua"
 dofile "../build/premake4-win/ext-portaudio.premake4.lua"
 dofile "../build/premake4-win/ext-portmidi.premake4.lua"
 dofile "../build/premake4-win/ext-pugixml.premake4.lua"
 dofile "../build/premake4-win/ext-r8brain.premake4.lua"
 dofile "../build/premake4-win/ext-smbPitchShift.premake4.lua"
 dofile "../build/premake4-win/ext-soundtouch.premake4.lua"
 dofile "../build/premake4-win/ext-UnRAR.premake4.lua"
 dofile "../build/premake4-win/ext-zlib.premake4.lua"
