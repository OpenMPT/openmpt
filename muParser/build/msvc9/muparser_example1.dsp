# Microsoft Developer Studio Project File - Name="muparser_example1" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=example1 - Win32 Release Static
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "muparser_example1.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "muparser_example1.mak" CFG="example1 - Win32 Release Static"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "example1 - Win32 Debug Static" (based on "Win32 (x86) Console Application")
!MESSAGE "example1 - Win32 Release Static" (based on "Win32 (x86) Console Application")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "example1 - Win32 Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\samples\example1"
# PROP BASE Intermediate_Dir "obj\vc_static_dbg\example1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\samples\example1"
# PROP Intermediate_Dir "obj\vc_static_dbg\example1"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\samples\example1\example1.pdb /D "WIN32" /D "_CONSOLE" /D "_DEBUG" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\samples\example1\example1.pdb /D "WIN32" /D "_CONSOLE" /D "_DEBUG" /c
# ADD BASE RSC /l 0x409 /d "_CONSOLE" /d "_DEBUG" /i ..\..\include
# ADD RSC /l 0x409 /d "_CONSOLE" /d "_DEBUG" /i ..\..\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example1\example1.exe" /subsystem:console /libpath:"..\..\lib" /debug
# ADD LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example1\example1.exe" /subsystem:console /libpath:"..\..\lib" /debug

!ELSEIF  "$(CFG)" == "example1 - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\samples\example1"
# PROP BASE Intermediate_Dir "obj\vc_static_rel\example1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\samples\example1"
# PROP Intermediate_Dir "obj\vc_static_rel\example1"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\samples\example1\example1.pdb /D "WIN32" /D "_CONSOLE" /D "NDEBUG" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\samples\example1\example1.pdb /D "WIN32" /D "_CONSOLE" /D "NDEBUG" /c
# ADD BASE RSC /l 0x409 /d "_CONSOLE" /d "NDEBUG" /i ..\..\include
# ADD RSC /l 0x409 /d "_CONSOLE" /d "NDEBUG" /i ..\..\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example1\example1.exe" /subsystem:console /libpath:"..\..\lib"
# ADD LINK32 ..\..\lib\muparser.lib /nologo /machine:i386 /out:"..\..\samples\example1\example1.exe" /subsystem:console /libpath:"..\..\lib"

!ENDIF

# Begin Target

# Name "example1 - Win32 Debug Static"
# Name "example1 - Win32 Release Static"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\samples\example1\Example1.cpp
# End Source File
# End Group
# End Target
# End Project

