# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=RayThrower - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to RayThrower - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "RayThrower - Win32 Release" && "$(CFG)" !=\
 "RayThrower - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "RayThrower.mak" CFG="RayThrower - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RayThrower - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "RayThrower - Win32 Debug" (based on\
 "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "RayThrower - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "RayThrower - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\RayThrower.exe"

CLEAN : 
	-@erase "$(INTDIR)\ambientmaterial.obj"
	-@erase "$(INTDIR)\camera.obj"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\cstring.obj"
	-@erase "$(INTDIR)\diffusematerial.obj"
	-@erase "$(INTDIR)\filestream.obj"
	-@erase "$(INTDIR)\imagefile.obj"
	-@erase "$(INTDIR)\light.obj"
	-@erase "$(INTDIR)\lightarray.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\material.obj"
	-@erase "$(INTDIR)\materiallist.obj"
	-@erase "$(INTDIR)\Matrix.obj"
	-@erase "$(INTDIR)\objects.obj"
	-@erase "$(INTDIR)\objectsarray.obj"
	-@erase "$(INTDIR)\plane.obj"
	-@erase "$(INTDIR)\ptlight.obj"
	-@erase "$(INTDIR)\pvmstream.obj"
	-@erase "$(INTDIR)\RayArray.obj"
	-@erase "$(INTDIR)\rays.obj"
	-@erase "$(INTDIR)\raysettings.obj"
	-@erase "$(INTDIR)\reflectivematerial.obj"
	-@erase "$(INTDIR)\scene.obj"
	-@erase "$(INTDIR)\specularmaterial.obj"
	-@erase "$(INTDIR)\sphere.obj"
	-@erase "$(INTDIR)\Transfrm.obj"
	-@erase "$(INTDIR)\transparentmaterial.obj"
	-@erase "$(INTDIR)\triangle.obj"
	-@erase "$(INTDIR)\vector.obj"
	-@erase "$(OUTDIR)\RayThrower.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "__THROWER__" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_CONSOLE"\
 /D "__THROWER__" /Fp"$(INTDIR)/RayThrower.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/RayThrower.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/RayThrower.pdb" /machine:I386 /out:"$(OUTDIR)/RayThrower.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ambientmaterial.obj" \
	"$(INTDIR)\camera.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\cstring.obj" \
	"$(INTDIR)\diffusematerial.obj" \
	"$(INTDIR)\filestream.obj" \
	"$(INTDIR)\imagefile.obj" \
	"$(INTDIR)\light.obj" \
	"$(INTDIR)\lightarray.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\material.obj" \
	"$(INTDIR)\materiallist.obj" \
	"$(INTDIR)\Matrix.obj" \
	"$(INTDIR)\objects.obj" \
	"$(INTDIR)\objectsarray.obj" \
	"$(INTDIR)\plane.obj" \
	"$(INTDIR)\ptlight.obj" \
	"$(INTDIR)\pvmstream.obj" \
	"$(INTDIR)\RayArray.obj" \
	"$(INTDIR)\rays.obj" \
	"$(INTDIR)\raysettings.obj" \
	"$(INTDIR)\reflectivematerial.obj" \
	"$(INTDIR)\scene.obj" \
	"$(INTDIR)\specularmaterial.obj" \
	"$(INTDIR)\sphere.obj" \
	"$(INTDIR)\Transfrm.obj" \
	"$(INTDIR)\transparentmaterial.obj" \
	"$(INTDIR)\triangle.obj" \
	"$(INTDIR)\vector.obj" \
	"..\Libfpvm3.lib" \
	"..\libgpvm3.lib" \
	"..\libpvm.lib"

"$(OUTDIR)\RayThrower.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "RayThrower - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\RayThrower.exe"

CLEAN : 
	-@erase "$(INTDIR)\ambientmaterial.obj"
	-@erase "$(INTDIR)\camera.obj"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\cstring.obj"
	-@erase "$(INTDIR)\diffusematerial.obj"
	-@erase "$(INTDIR)\filestream.obj"
	-@erase "$(INTDIR)\imagefile.obj"
	-@erase "$(INTDIR)\light.obj"
	-@erase "$(INTDIR)\lightarray.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\material.obj"
	-@erase "$(INTDIR)\materiallist.obj"
	-@erase "$(INTDIR)\Matrix.obj"
	-@erase "$(INTDIR)\objects.obj"
	-@erase "$(INTDIR)\objectsarray.obj"
	-@erase "$(INTDIR)\plane.obj"
	-@erase "$(INTDIR)\ptlight.obj"
	-@erase "$(INTDIR)\pvmstream.obj"
	-@erase "$(INTDIR)\RayArray.obj"
	-@erase "$(INTDIR)\rays.obj"
	-@erase "$(INTDIR)\raysettings.obj"
	-@erase "$(INTDIR)\reflectivematerial.obj"
	-@erase "$(INTDIR)\scene.obj"
	-@erase "$(INTDIR)\specularmaterial.obj"
	-@erase "$(INTDIR)\sphere.obj"
	-@erase "$(INTDIR)\Transfrm.obj"
	-@erase "$(INTDIR)\transparentmaterial.obj"
	-@erase "$(INTDIR)\triangle.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\vector.obj"
	-@erase "$(OUTDIR)\RayThrower.exe"
	-@erase "$(OUTDIR)\RayThrower.ilk"
	-@erase "$(OUTDIR)\RayThrower.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "__THROWER__" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D\
 "_CONSOLE" /D "__THROWER__" /Fp"$(INTDIR)/RayThrower.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/RayThrower.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/RayThrower.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/RayThrower.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ambientmaterial.obj" \
	"$(INTDIR)\camera.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\cstring.obj" \
	"$(INTDIR)\diffusematerial.obj" \
	"$(INTDIR)\filestream.obj" \
	"$(INTDIR)\imagefile.obj" \
	"$(INTDIR)\light.obj" \
	"$(INTDIR)\lightarray.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\material.obj" \
	"$(INTDIR)\materiallist.obj" \
	"$(INTDIR)\Matrix.obj" \
	"$(INTDIR)\objects.obj" \
	"$(INTDIR)\objectsarray.obj" \
	"$(INTDIR)\plane.obj" \
	"$(INTDIR)\ptlight.obj" \
	"$(INTDIR)\pvmstream.obj" \
	"$(INTDIR)\RayArray.obj" \
	"$(INTDIR)\rays.obj" \
	"$(INTDIR)\raysettings.obj" \
	"$(INTDIR)\reflectivematerial.obj" \
	"$(INTDIR)\scene.obj" \
	"$(INTDIR)\specularmaterial.obj" \
	"$(INTDIR)\sphere.obj" \
	"$(INTDIR)\Transfrm.obj" \
	"$(INTDIR)\transparentmaterial.obj" \
	"$(INTDIR)\triangle.obj" \
	"$(INTDIR)\vector.obj" \
	"..\Libfpvm3.lib" \
	"..\libgpvm3.lib" \
	"..\libpvm.lib"

"$(OUTDIR)\RayThrower.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "RayThrower - Win32 Release"
# Name "RayThrower - Win32 Debug"

!IF  "$(CFG)" == "RayThrower - Win32 Release"

!ELSEIF  "$(CFG)" == "RayThrower - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=\Projects\ray\vector.cpp
DEP_CPP_VECTO=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\vector.obj" : $(SOURCE) $(DEP_CPP_VECTO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\color.cpp
DEP_CPP_COLOR=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\color.obj" : $(SOURCE) $(DEP_CPP_COLOR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\diffusematerial.cpp
DEP_CPP_DIFFU=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\diffusematerialpropdlg.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\diffusematerial.obj" : $(SOURCE) $(DEP_CPP_DIFFU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\filestream.cpp
DEP_CPP_FILES=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\filestream.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\filestream.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\light.cpp
DEP_CPP_LIGHT=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\light.obj" : $(SOURCE) $(DEP_CPP_LIGHT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\lightarray.cpp
DEP_CPP_LIGHTA=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\lightarray.obj" : $(SOURCE) $(DEP_CPP_LIGHTA) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\material.cpp
DEP_CPP_MATER=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\material.obj" : $(SOURCE) $(DEP_CPP_MATER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\materiallist.cpp
DEP_CPP_MATERI=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\materiallist.obj" : $(SOURCE) $(DEP_CPP_MATERI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\Matrix.cpp
DEP_CPP_MATRI=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\Matrix.obj" : $(SOURCE) $(DEP_CPP_MATRI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\objects.cpp
DEP_CPP_OBJEC=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\materialsdlg.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\objects.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\objectsarray.cpp
DEP_CPP_OBJECT=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\objectsarray.obj" : $(SOURCE) $(DEP_CPP_OBJECT) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\plane.cpp
DEP_CPP_PLANE=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\plane.h"\
	"..\planepropertiesdlg.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\plane.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\ptlight.cpp
DEP_CPP_PTLIG=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\pointlightpropertiesdlg.h"\
	"..\ptlight.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\ptlight.obj" : $(SOURCE) $(DEP_CPP_PTLIG) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\pvmstream.cpp
DEP_CPP_PVMST=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\pvm3.h"\
	"..\pvmstream.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	

"$(INTDIR)\pvmstream.obj" : $(SOURCE) $(DEP_CPP_PVMST) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\rays.cpp
DEP_CPP_RAYS_=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\transfrm.h"\
	"..\vector.h"\
	

"$(INTDIR)\rays.obj" : $(SOURCE) $(DEP_CPP_RAYS_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\raysettings.cpp
DEP_CPP_RAYSE=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\settingsdialog.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\raysettings.obj" : $(SOURCE) $(DEP_CPP_RAYSE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\reflectivematerial.cpp
DEP_CPP_REFLE=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\reflectivematdlg.h"\
	"..\reflectivematerial.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\reflectivematerial.obj" : $(SOURCE) $(DEP_CPP_REFLE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\scene.cpp
DEP_CPP_SCENE=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\scene.obj" : $(SOURCE) $(DEP_CPP_SCENE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\specularmaterial.cpp
DEP_CPP_SPECU=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\specmatdlg.h"\
	"..\Specularmaterial.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\specularmaterial.obj" : $(SOURCE) $(DEP_CPP_SPECU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\sphere.cpp
DEP_CPP_SPHER=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\sphere.h"\
	"..\SpherePropertiesDlg.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\sphere.obj" : $(SOURCE) $(DEP_CPP_SPHER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\Transfrm.cpp
DEP_CPP_TRANS=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\transfrm.h"\
	"..\vector.h"\
	

"$(INTDIR)\Transfrm.obj" : $(SOURCE) $(DEP_CPP_TRANS) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\camera.cpp
DEP_CPP_CAMER=\
	"..\camera.h"\
	"..\camerapropertiesdlg.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\camera.obj" : $(SOURCE) $(DEP_CPP_CAMER) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cstring.cpp
DEP_CPP_CSTRI=\
	"..\globals.h"\
	"..\rayengine\cstring.h"\
	

"$(INTDIR)\cstring.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "RayThrower - Win32 Release"

DEP_CPP_MAIN_=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\filestream.h"\
	"..\globals.h"\
	"..\imagefile.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\msgtag.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\pvm3.h"\
	"..\pvmstream.h"\
	"..\rayarray.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stream.h"\
	"..\vector.h"\
	".\cstring.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	
NODEP_CPP_MAIN_=\
	".\pvm3.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "RayThrower - Win32 Debug"

DEP_CPP_MAIN_=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\filestream.h"\
	"..\globals.h"\
	"..\imagefile.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\msgtag.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\pvm3.h"\
	"..\pvmstream.h"\
	"..\rayarray.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stream.h"\
	"..\vector.h"\
	".\cstring.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\imagefile.cpp
DEP_CPP_IMAGE=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\imagefile.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\imagefile.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\libpvm.lib

!IF  "$(CFG)" == "RayThrower - Win32 Release"

!ELSEIF  "$(CFG)" == "RayThrower - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\libgpvm3.lib

!IF  "$(CFG)" == "RayThrower - Win32 Release"

!ELSEIF  "$(CFG)" == "RayThrower - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\Libfpvm3.lib

!IF  "$(CFG)" == "RayThrower - Win32 Release"

!ELSEIF  "$(CFG)" == "RayThrower - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\triangle.cpp
DEP_CPP_TRIAN=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\triangle.h"\
	"..\vector.h"\
	

"$(INTDIR)\triangle.obj" : $(SOURCE) $(DEP_CPP_TRIAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\RayArray.cpp
DEP_CPP_RAYAR=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\rayarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\RayArray.obj" : $(SOURCE) $(DEP_CPP_RAYAR) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\transparentmaterial.cpp
DEP_CPP_TRANSP=\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\transparentmatdlg.h"\
	"..\transparentmaterial.h"\
	"..\vector.h"\
	

"$(INTDIR)\transparentmaterial.obj" : $(SOURCE) $(DEP_CPP_TRANSP) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\Projects\ray\ambientmaterial.cpp
DEP_CPP_AMBIE=\
	"..\AmbientMaterial.h"\
	"..\camera.h"\
	"..\color.h"\
	"..\diffusematerial.h"\
	"..\globals.h"\
	"..\Light.h"\
	"..\lightarray.h"\
	"..\material.h"\
	"..\materiallist.h"\
	"..\matrix.h"\
	"..\objects.h"\
	"..\objectsarray.h"\
	"..\raydoc.h"\
	"..\rayengine\cstring.h"\
	"..\rays.h"\
	"..\raysettings.h"\
	"..\scene.h"\
	"..\scenetreeview.h"\
	"..\SetColordlg.h"\
	"..\stdafx.h"\
	"..\stream.h"\
	"..\vector.h"\
	

"$(INTDIR)\ambientmaterial.obj" : $(SOURCE) $(DEP_CPP_AMBIE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
