# Microsoft Developer Studio Project File - Name="cubic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=cubic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cubic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cubic.mak" CFG="cubic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cubic - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cubic - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cubic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../bin"
# PROP BASE Intermediate_Dir "static/Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "static/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W1 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /O2 /I "../../include" /I "../include" /I "..\..\src\air" /I "..\..\src\hest" /I "..\..\src\biff" /I "..\..\src\nrrd" /I "..\..\src\ell" /I "..\..\src\unrrdu" /I "..\..\src\dye" /I "..\..\src\moss" /I "..\..\src\gage" /I "..\..\src\bane" /I "..\..\src\limn" /I "..\..\src\hoover" /I "..\..\src\mite" /I "..\..\src\echo" /I "..\..\src\ten" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "TEEM_ENDIAN=1234" /D "TEEM_QNANHIBIT=1" /D "TEEM_DIO=0" /D "TEEM_32BIT=1" /D "TEEM_BIGBITFIELD=1" /D "TEEM_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:libcmt
# ADD LINK32 /libpath:"../lib/static" teem.lib png.lib bz2.lib z.lib /nologo /subsystem:console /pdb:none /machine:I386
# Begin Special Build Tool
TargetName=cubic
SOURCE="$(InputPath)"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "cubic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "../bin"
# PROP BASE Intermediate_Dir "static/Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "static/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W1 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MD /Zi /Od /I "../../include" /I "../include" /I "..\..\src\air" /I "..\..\src\hest" /I "..\..\src\biff" /I "..\..\src\nrrd" /I "..\..\src\ell" /I "..\..\src\unrrdu" /I "..\..\src\dye" /I "..\..\src\moss" /I "..\..\src\gage" /I "..\..\src\bane" /I "..\..\src\limn" /I "..\..\src\hoover" /I "..\..\src\mite" /I "..\..\src\echo" /I "..\..\src\ten" /D "WIN32" /D "WIN32_MEAN_AND_LEAN" /D "VC_EXTRALEAN" /D "TEEM_ENDIAN=1234" /D "TEEM_QNANHIBIT=1" /D "TEEM_DIO=0" /D "TEEM_32BIT=1" /D "TEEM_BIGBITFIELD=1" /D "TEEM_STATIC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:libcmt /pdbtype:sept
# ADD LINK32 /libpath:"../lib/static" teem_d.lib png.lib bz2.lib z.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept /pdb:../bin/cubic_d.pdb /out:../bin/cubic_d.exe
# Begin Special Build Tool
TargetName=cubic_d
SOURCE="$(InputPath)"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "cubic - Win32 Release"
# Name "cubic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\bin\cubic.c
# End Source File
# End Group
# End Target
# End Project
