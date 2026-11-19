# Microsoft Developer Studio Project File - Name="Ray" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Ray - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Ray.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ray.mak" CFG="Ray - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ray - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Ray - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Ray - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /I ".\pvm3\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcd.lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".\pvm3\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "Ray - Win32 Release"
# Name "Ray - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\AddMaterialsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AddPlaneDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AddPtLightDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\AddSphereDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ambientmaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\camera.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraPropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\color.cpp
# End Source File
# Begin Source File

SOURCE=.\colorarray.cpp
# End Source File
# Begin Source File

SOURCE=.\diffusematerial.cpp
# End Source File
# Begin Source File

SOURCE=.\DiffuseMaterialPropDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\filestream.cpp
# End Source File
# Begin Source File

SOURCE=.\imagefile.cpp
# End Source File
# Begin Source File

SOURCE=.\InfluenceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\light.cpp
# End Source File
# Begin Source File

SOURCE=.\lightarray.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\material.cpp
# End Source File
# Begin Source File

SOURCE=.\materiallist.cpp
# End Source File
# Begin Source File

SOURCE=.\MaterialsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Matrix.cpp
# End Source File
# Begin Source File

SOURCE=.\objects.cpp
# End Source File
# Begin Source File

SOURCE=.\objectsarray.cpp
# End Source File
# Begin Source File

SOURCE=.\pipestream.cpp
# End Source File
# Begin Source File

SOURCE=.\plane.cpp
# End Source File
# Begin Source File

SOURCE=.\PlanePropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PointLightPropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ptlight.cpp
# End Source File
# Begin Source File

SOURCE=.\Ray.cpp
# End Source File
# Begin Source File

SOURCE=.\hlp\Ray.hpj

!IF  "$(CFG)" == "Ray - Win32 Release"

# Begin Custom Build - Making help file...
OutDir=.\.\Release
ProjDir=.
TargetName=Ray
InputPath=.\hlp\Ray.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

# Begin Custom Build - Making help file...
OutDir=.\.\Debug
ProjDir=.
TargetName=Ray
InputPath=.\hlp\Ray.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(ProjDir)\makehelp.bat"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Ray.rc
# End Source File
# Begin Source File

SOURCE=.\RayArray.cpp
# End Source File
# Begin Source File

SOURCE=.\RayDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\rays.cpp
# End Source File
# Begin Source File

SOURCE=.\raysettings.cpp
# End Source File
# Begin Source File

SOURCE=.\RayView.cpp
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\ReflectiveMatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\reflectivematerial.cpp
# End Source File
# Begin Source File

SOURCE=.\scene.cpp
# End Source File
# Begin Source File

SOURCE=.\scenetreeview.cpp
# End Source File
# Begin Source File

SOURCE=.\semaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\SetColorDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\SpecMatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\specularmaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\sphere.cpp
# End Source File
# Begin Source File

SOURCE=.\SpherePropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\taskmaster.cpp
# End Source File
# Begin Source File

SOURCE=.\taskslave.cpp
# End Source File
# Begin Source File

SOURCE=.\threadtaskmaster.cpp
# End Source File
# Begin Source File

SOURCE=.\threadtaskslave.cpp
# End Source File
# Begin Source File

SOURCE=.\Transfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\TransparentMatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\transparentmaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\triangle.cpp
# End Source File
# Begin Source File

SOURCE=.\vector.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\AddMaterialsDlg.h
# End Source File
# Begin Source File

SOURCE=.\AddPlaneDlg.h
# End Source File
# Begin Source File

SOURCE=.\AddPtLightDlg.h
# End Source File
# Begin Source File

SOURCE=.\AddSphereDlg.h
# End Source File
# Begin Source File

SOURCE=.\ambientmaterial.h
# End Source File
# Begin Source File

SOURCE=.\camera.h
# End Source File
# Begin Source File

SOURCE=.\CameraPropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\color.h
# End Source File
# Begin Source File

SOURCE=.\colorarray.h
# End Source File
# Begin Source File

SOURCE=.\diffusematerial.h
# End Source File
# Begin Source File

SOURCE=.\DiffuseMaterialPropDlg.h
# End Source File
# Begin Source File

SOURCE=.\filestream.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\ImageFile.h
# End Source File
# Begin Source File

SOURCE=.\InfluenceDlg.h
# End Source File
# Begin Source File

SOURCE=.\light.h
# End Source File
# Begin Source File

SOURCE=.\lightarray.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\material.h
# End Source File
# Begin Source File

SOURCE=.\materiallist.h
# End Source File
# Begin Source File

SOURCE=.\MaterialsDlg.h
# End Source File
# Begin Source File

SOURCE=.\Matrix.h
# End Source File
# Begin Source File

SOURCE=.\msgtag.h
# End Source File
# Begin Source File

SOURCE=.\objects.h
# End Source File
# Begin Source File

SOURCE=.\objectsarray.h
# End Source File
# Begin Source File

SOURCE=.\pipestream.h
# End Source File
# Begin Source File

SOURCE=.\plane.h
# End Source File
# Begin Source File

SOURCE=.\PlanePropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\PointLightPropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\ptlight.h
# End Source File
# Begin Source File

SOURCE=.\pvm3.h
# End Source File
# Begin Source File

SOURCE=.\pvmstream.h
# End Source File
# Begin Source File

SOURCE=.\pvmtaskmaster.h
# End Source File
# Begin Source File

SOURCE=.\pvmTaskSlave.h
# End Source File
# Begin Source File

SOURCE=.\Ray.h
# End Source File
# Begin Source File

SOURCE=.\RayArray.h
# End Source File
# Begin Source File

SOURCE=.\RayDoc.h
# End Source File
# Begin Source File

SOURCE=.\rays.h
# End Source File
# Begin Source File

SOURCE=.\RaySettings.h
# End Source File
# Begin Source File

SOURCE=.\RayView.h
# End Source File
# Begin Source File

SOURCE=.\ReflectiveMatDlg.h
# End Source File
# Begin Source File

SOURCE=.\reflectivematerial.h
# End Source File
# Begin Source File

SOURCE=.\scene.h
# End Source File
# Begin Source File

SOURCE=.\scenetreeview.h
# End Source File
# Begin Source File

SOURCE=.\semaphore.h
# End Source File
# Begin Source File

SOURCE=.\SetColorDlg.h
# End Source File
# Begin Source File

SOURCE=.\SettingsDialog.h
# End Source File
# Begin Source File

SOURCE=.\SpecMatDlg.h
# End Source File
# Begin Source File

SOURCE=.\SpecularMaterial.h
# End Source File
# Begin Source File

SOURCE=.\sphere.h
# End Source File
# Begin Source File

SOURCE=.\SpherePropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\stream.h
# End Source File
# Begin Source File

SOURCE=.\taskmaster.h
# End Source File
# Begin Source File

SOURCE=.\taskslave.h
# End Source File
# Begin Source File

SOURCE=.\Threadtaskmaster.h
# End Source File
# Begin Source File

SOURCE=.\ThreadTaskSlave.h
# End Source File
# Begin Source File

SOURCE=.\Transfrm.h
# End Source File
# Begin Source File

SOURCE=.\TransparentMatDlg.h
# End Source File
# Begin Source File

SOURCE=.\transparentmaterial.h
# End Source File
# Begin Source File

SOURCE=.\triangle.h
# End Source File
# Begin Source File

SOURCE=.\vector.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\about_bi.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Ray.ico
# End Source File
# Begin Source File

SOURCE=.\res\Ray.rc2
# End Source File
# Begin Source File

SOURCE=.\res\RayDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\scene_tr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\small_ic.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
