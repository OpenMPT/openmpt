# Microsoft Developer Studio Project File - Name="mptrack" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=mptrack - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mptrack.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mptrack.mak" CFG="mptrack - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mptrack - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mptrack - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mptrack - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Bin"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "..\unlha" /I "..\unzip" /I "..\unrar" /I "..\soundlib" /I "..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ENABLE_MMX" /D "ENABLE_EQ" /D "MODPLUG_TRACKER" /D "NO_PACKING" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib strmiids.lib dmoguids.lib /nologo /version:5.0 /pdb:none /map /machine:I386 /subsystem:windows,4.0
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "mptrack - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G5 /MTd /W4 /Gm- /GX /Zi /Od /Gf /Gy /I "..\unzip" /I "..\unrar" /I "..\unlha" /I "..\soundlib" /I "..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ENABLE_MMX" /D "ENABLE_EQ" /D "MODPLUG_TRACKER" /D "NO_PACKING" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib strmiids.lib dmoguids.lib /nologo /version:5.0 /subsystem:windows /incremental:no /map /debug /machine:I386 /libpath:"..\unzip\bin" /libpath:"..\unrar\bin" /libpath:"..\unlha\bin"
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "mptrack - Win32 Release"
# Name "mptrack - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrl_com.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrl_gen.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrl_ins.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrl_pat.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrl_seq.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrl_smp.cpp
# End Source File
# Begin Source File

SOURCE=.\dlg_misc.cpp

!IF  "$(CFG)" == "mptrack - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "mptrack - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\soundlib\dlsbank.cpp
# End Source File
# Begin Source File

SOURCE=.\draw_pat.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Fastmix.cpp
# End Source File
# Begin Source File

SOURCE=.\globals.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_669.cpp
# End Source File
# Begin Source File

SOURCE=..\Soundlib\load_amf.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_ams.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\load_dbm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\load_dmf.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_dsm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_far.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_it.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_mdl.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_med.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\load_mid.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_mod.cpp
# End Source File
# Begin Source File

SOURCE=..\Soundlib\load_mt2.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_mtm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_okt.cpp
# End Source File
# Begin Source File

SOURCE=..\Soundlib\load_psm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\load_ptm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_s3m.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_stm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_ult.cpp
# End Source File
# Begin Source File

SOURCE=..\Soundlib\Load_umx.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_wav.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Load_xm.cpp
# End Source File
# Begin Source File

SOURCE=.\mainbar.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\mmcmp.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Mmx_mix.cpp
# End Source File
# Begin Source File

SOURCE=.\mod2midi.cpp
# End Source File
# Begin Source File

SOURCE=.\mod2wave.cpp
# End Source File
# Begin Source File

SOURCE=.\moddoc.cpp
# End Source File
# Begin Source File

SOURCE=.\modedit.cpp
# End Source File
# Begin Source File

SOURCE=.\Moptions.cpp
# End Source File
# Begin Source File

SOURCE=.\Mpdlgs.cpp
# End Source File
# Begin Source File

SOURCE=.\mpt_midi.cpp
# End Source File
# Begin Source File

SOURCE=.\mptrack.cpp
# End Source File
# Begin Source File

SOURCE=.\mptrack.rc
# End Source File
# Begin Source File

SOURCE=..\soundlib\Sampleio.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\snd_dsp.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\snd_eq.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\snd_flt.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Snd_fx.cpp
# End Source File
# Begin Source File

SOURCE=..\Soundlib\Snd_rvb.cpp

!IF  "$(CFG)" == "mptrack - Win32 Release"

# ADD CPP /FAs

!ELSEIF  "$(CFG)" == "mptrack - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Soundlib\snddev.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Sndfile.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Sndmix.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\soundlib\Tables.cpp
# End Source File
# Begin Source File

SOURCE=.\view_com.cpp
# End Source File
# Begin Source File

SOURCE=.\view_gen.cpp
# End Source File
# Begin Source File

SOURCE=.\view_ins.cpp
# End Source File
# Begin Source File

SOURCE=.\view_pat.cpp
# End Source File
# Begin Source File

SOURCE=.\view_smp.cpp
# End Source File
# Begin Source File

SOURCE=.\view_tre.cpp
# End Source File
# Begin Source File

SOURCE=.\vstplug.cpp
# End Source File
# Begin Source File

SOURCE=..\soundlib\Waveform.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\ctrl_com.h
# End Source File
# Begin Source File

SOURCE=.\ctrl_gen.h
# End Source File
# Begin Source File

SOURCE=.\ctrl_ins.h
# End Source File
# Begin Source File

SOURCE=.\ctrl_pat.h
# End Source File
# Begin Source File

SOURCE=.\ctrl_smp.h
# End Source File
# Begin Source File

SOURCE=.\dlg_misc.h
# End Source File
# Begin Source File

SOURCE=..\Soundlib\Dlsbank.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\mainbar.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\mod2midi.h
# End Source File
# Begin Source File

SOURCE=.\mod2wave.h
# End Source File
# Begin Source File

SOURCE=.\moddoc.h
# End Source File
# Begin Source File

SOURCE=.\Moptions.h
# End Source File
# Begin Source File

SOURCE=.\Mpdlgs.h
# End Source File
# Begin Source File

SOURCE=.\mptrack.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\Soundlib\snddev.h
# End Source File
# Begin Source File

SOURCE=..\Soundlib\snddevx.h
# End Source File
# Begin Source File

SOURCE=..\Soundlib\Sndfile.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\view_com.h
# End Source File
# Begin Source File

SOURCE=.\view_gen.h
# End Source File
# Begin Source File

SOURCE=.\view_ins.h
# End Source File
# Begin Source File

SOURCE=.\view_pat.h
# End Source File
# Begin Source File

SOURCE=.\view_smp.h
# End Source File
# Begin Source File

SOURCE=.\view_tre.h
# End Source File
# Begin Source File

SOURCE=.\vstplug.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\colors.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dragging.cur
# End Source File
# Begin Source File

SOURCE=.\Res\envbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\img_list.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mainbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\moddoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\MPTRACK.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mptrack.ico
# End Source File
# Begin Source File

SOURCE=.\res\mptrack.rc2
# End Source File
# Begin Source File

SOURCE=.\Res\nodrag.cur
# End Source File
# Begin Source File

SOURCE=.\res\nodrop.cur
# End Source File
# Begin Source File

SOURCE=.\res\patterns.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\smptoolb.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\splash.bmp
# End Source File
# Begin Source File

SOURCE=.\res\view_pat.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vumeters.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\mptrack.reg
# End Source File
# End Target
# End Project
