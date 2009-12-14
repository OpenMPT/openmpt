# Microsoft Developer Studio Project File - Name="muparser_example3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=example3 - Win32 Release DLL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "muparser_example3.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "muparser_example3.mak" CFG="example3 - Win32 Release DLL"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "example3 - Win32 Debug DLL" (based on "Win32 (x86) Console Application")
!MESSAGE "example3 - Win32 Release DLL" (based on "Win32 (x86) Console Application")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "example3 - Win32 Debug DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\samples\example3"
# PROP BASE Intermediate_Dir "obj\vc_shared_dbg\example3"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\samples\example3"
# PROP Intermediate_Dir "obj\vc_shared_dbg\example3"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\samples\example3\example3.pdb /D "WIN32" /D "_CONSOLE" /D "_DEBUG" /D "USINGDLL" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\samples\example3\example3.pdb /D "WIN32" /D "_CONSOLE" /D "_DEBUG" /D "USINGDLL" /c
# ADD BASE RSC /l 0x409 /d "_CONSOLE" /d "_DEBUG" /i "..\..\include" /d USINGDLL
# ADD RSC /l 0x409 /d "_CONSOLE" /d "_DEBUG" /i "..\..\include" /d USINGDLL
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example3\example3.exe" /subsystem:console /libpath:"..\..\lib" /debug
# ADD LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example3\example3.exe" /subsystem:console /libpath:"..\..\lib" /debug

!ELSEIF  "$(CFG)" == "example3 - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\samples\example3"
# PROP BASE Intermediate_Dir "obj\vc_shared_rel\example3"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\samples\example3"
# PROP Intermediate_Dir "obj\vc_shared_rel\example3"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\samples\example3\example3.pdb /D "WIN32" /D "_CONSOLE" /D "NDEBUG" /D "USINGDLL" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\samples\example3\example3.pdb /D "WIN32" /D "_CONSOLE" /D "NDEBUG" /D "USINGDLL" /c
# ADD BASE RSC /l 0x409 /d "_CONSOLE" /d "NDEBUG" /i "..\..\include" /d USINGDLL
# ADD RSC /l 0x409 /d "_CONSOLE" /d "NDEBUG" /i "..\..\include" /d USINGDLL
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example3\example3.exe" /subsystem:console /libpath:"..\..\lib"
# ADD LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example3\example3.exe" /subsystem:console /libpath:"..\..\lib"

!ENDIF

# Begin Target

# Name "example3 - Win32 Debug DLL"
# Name "example3 - Win32 Release DLL"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\samples\example3\Example3.cpp
# End Source File
# End Group
# End Target
# End Project

