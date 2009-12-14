# Microsoft Developer Studio Project File - Name="muparser_muParser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=muParser - Win32 Release Static
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "muparser_muParser.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "muparser_muParser.mak" CFG="muParser - Win32 Release Static"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "muParser - Win32 Debug DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "muParser - Win32 Debug Static" (based on "Win32 (x86) Static Library")
!MESSAGE "muParser - Win32 Release DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "muParser - Win32 Release Static" (based on "Win32 (x86) Static Library")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "muParser - Win32 Debug DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\lib"
# PROP BASE Intermediate_Dir "obj\vc_shared_dbg\muParser"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "obj\vc_shared_dbg\muParser"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "_DEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "_DEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "_DEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "_DEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "MUPARSER_DLL" /d "MUPARSERLIB_EXPORTS" /d "_WIN32" /i ..\..\include
# ADD RSC /l 0x409 /d "_DEBUG" /d "MUPARSER_DLL" /d "MUPARSERLIB_EXPORTS" /d "_WIN32" /i ..\..\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /machine:i386 /out:"..\..\lib\muparser.dll" /implib:"..\..\lib\muparser.lib" /debug
# ADD LINK32 /nologo /dll /machine:i386 /out:"..\..\lib\muparser.dll" /implib:"..\..\lib\muparser.lib" /debug

!ELSEIF  "$(CFG)" == "muParser - Win32 Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\lib"
# PROP BASE Intermediate_Dir "obj\vc_static_dbg\muParser"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "obj\vc_static_dbg\muParser"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_LIB" /D "_DEBUG" /D "_WIN32" /c
# ADD CPP /nologo /FD /MDd /GR /EHsc /Od /W4 /I "..\..\include" /Zi /Gm /GZ /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_LIB" /D "_DEBUG" /D "_WIN32" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\muparser.lib"
# ADD LIB32 /nologo /out:"..\..\lib\muparser.lib"

!ELSEIF  "$(CFG)" == "muParser - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\lib"
# PROP BASE Intermediate_Dir "obj\vc_shared_rel\muParser"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "obj\vc_shared_rel\muParser"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "NDEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "NDEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /c
# ADD BASE MTL /nologo /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "NDEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /mktyplib203 /win32
# ADD MTL /nologo /D "WIN32" /D "_USRDLL" /D "DLL_EXPORTS" /D "NDEBUG" /D "MUPARSER_DLL" /D "MUPARSERLIB_EXPORTS" /D "_WIN32" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MUPARSER_DLL" /d "MUPARSERLIB_EXPORTS" /d "_WIN32" /i ..\..\include
# ADD RSC /l 0x409 /d "NDEBUG" /d "MUPARSER_DLL" /d "MUPARSERLIB_EXPORTS" /d "_WIN32" /i ..\..\include
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /machine:i386 /out:"..\..\lib\muparser.dll" /implib:"..\..\lib\muparser.lib"
# ADD LINK32 /nologo /dll /machine:i386 /out:"..\..\lib\muparser.dll" /implib:"..\..\lib\muparser.lib"

!ELSEIF  "$(CFG)" == "muParser - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\lib"
# PROP BASE Intermediate_Dir "obj\vc_static_rel\muParser"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\lib"
# PROP Intermediate_Dir "obj\vc_static_rel\muParser"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_LIB" /D "NDEBUG" /D "_WIN32" /c
# ADD CPP /nologo /FD /MD /GR /EHsc /O2 /w /I "..\..\include" /Fd..\..\lib\muparser.pdb /D "WIN32" /D "_LIB" /D "NDEBUG" /D "_WIN32" /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\muparser.lib"
# ADD LIB32 /nologo /out:"..\..\lib\muparser.lib"

!ENDIF

# Begin Target

# Name "muParser - Win32 Debug DLL"
# Name "muParser - Win32 Debug Static"
# Name "muParser - Win32 Release DLL"
# Name "muParser - Win32 Release Static"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\muParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserBase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserBytecode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserDLL.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserError.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserInt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserTest.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\muParserTokenReader.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\muParser.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserBase.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserBytecode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserDLL.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserDef.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserError.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserFixes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserInt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserStack.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserTest.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserToken.h
# End Source File
# Begin Source File

SOURCE=..\..\include\muParserTokenReader.h
# End Source File
# End Group
# End Target
# End Project

