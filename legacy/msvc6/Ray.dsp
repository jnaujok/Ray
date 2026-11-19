# Microsoft Developer Studio Project File - Name="Ray" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
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
# PROP AllowPerConfigDependencies 0
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
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /I ".\include" /I ".\third_party\pvm3\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i ".\include" /d "NDEBUG" /d "_AFXDLL"
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
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".\include" /I ".\third_party\pvm3\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i ".\include" /d "_DEBUG" /d "_AFXDLL"
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

SOURCE=.\src\AddMaterialsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AddPlaneDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AddPtLightDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AddSphereDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\AmbientMaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Camera.cpp
# End Source File
# Begin Source File

SOURCE=.\src\CameraPropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Color.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ColorArray.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DiffuseMaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\src\DiffuseMaterialPropDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\FileStream.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ImageFile.cpp
# End Source File
# Begin Source File

SOURCE=.\src\InfluenceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Light.cpp
# End Source File
# Begin Source File

SOURCE=.\src\LightArray.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Material.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MaterialList.cpp
# End Source File
# Begin Source File

SOURCE=.\src\MaterialsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Matrix.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Objects.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ObjectsArray.cpp
# End Source File
# Begin Source File

SOURCE=.\src\PipeStream.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Plane.cpp
# End Source File
# Begin Source File

SOURCE=.\src\PlanePropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\PointLightPropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\PtLight.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Ray.cpp
# End Source File
# Begin Source File

SOURCE=.\help\Ray.hpj

!IF  "$(CFG)" == "Ray - Win32 Release"

# Begin Custom Build - Making help file...
OutDir=.\Release
ProjDir=.
TargetName=Ray
InputPath=.\help\Ray.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(ProjDir)\help\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

# Begin Custom Build - Making help file...
OutDir=.\Debug
ProjDir=.
TargetName=Ray
InputPath=.\help\Ray.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(ProjDir)\help\makehelp.bat"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\Ray.rc
# End Source File
# Begin Source File

SOURCE=.\src\RayArray.cpp
# End Source File
# Begin Source File

SOURCE=.\src\RayDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Rays.cpp
# End Source File
# Begin Source File

SOURCE=.\src\RaySettings.cpp
# End Source File
# Begin Source File

SOURCE=.\src\RayView.cpp
# End Source File
# Begin Source File

SOURCE=.\docs\AppWizardReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\src\ReflectiveMatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ReflectiveMaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Scene.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SceneTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Semaphore.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SetColorDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SettingsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SpecMatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SpecularMaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Sphere.cpp
# End Source File
# Begin Source File

SOURCE=.\src\SpherePropertiesDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\src\TaskMaster.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TaskSlave.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ThreadTaskMaster.cpp
# End Source File
# Begin Source File

SOURCE=.\src\ThreadTaskSlave.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Transform.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TransparentMatDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\TransparentMaterial.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Triangle.cpp
# End Source File
# Begin Source File

SOURCE=.\src\Vector.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\include\AddMaterialsDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\AddPlaneDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\AddPtLightDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\AddSphereDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\AmbientMaterial.h
# End Source File
# Begin Source File

SOURCE=.\include\Camera.h
# End Source File
# Begin Source File

SOURCE=.\include\CameraPropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\include\Color.h
# End Source File
# Begin Source File

SOURCE=.\include\ColorArray.h
# End Source File
# Begin Source File

SOURCE=.\include\DiffuseMaterial.h
# End Source File
# Begin Source File

SOURCE=.\include\DiffuseMaterialPropDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\FileStream.h
# End Source File
# Begin Source File

SOURCE=.\include\Globals.h
# End Source File
# Begin Source File

SOURCE=.\include\ImageFile.h
# End Source File
# Begin Source File

SOURCE=.\include\InfluenceDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\Light.h
# End Source File
# Begin Source File

SOURCE=.\include\LightArray.h
# End Source File
# Begin Source File

SOURCE=.\include\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\include\Material.h
# End Source File
# Begin Source File

SOURCE=.\include\MaterialList.h
# End Source File
# Begin Source File

SOURCE=.\include\MaterialsDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\Matrix.h
# End Source File
# Begin Source File

SOURCE=.\include\MsgTag.h
# End Source File
# Begin Source File

SOURCE=.\include\Objects.h
# End Source File
# Begin Source File

SOURCE=.\include\ObjectsArray.h
# End Source File
# Begin Source File

SOURCE=.\include\PipeStream.h
# End Source File
# Begin Source File

SOURCE=.\include\Plane.h
# End Source File
# Begin Source File

SOURCE=.\include\PlanePropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\PointLightPropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\PtLight.h
# End Source File
# Begin Source File

SOURCE=.\third_party\pvm3\include\pvm3.h
# End Source File
# Begin Source File

SOURCE=.\include\PVMStream.h
# End Source File
# Begin Source File

SOURCE=.\include\PVMTaskMaster.h
# End Source File
# Begin Source File

SOURCE=.\include\PVMTaskSlave.h
# End Source File
# Begin Source File

SOURCE=.\include\Ray.h
# End Source File
# Begin Source File

SOURCE=.\include\RayArray.h
# End Source File
# Begin Source File

SOURCE=.\include\RayDoc.h
# End Source File
# Begin Source File

SOURCE=.\include\Rays.h
# End Source File
# Begin Source File

SOURCE=.\include\RaySettings.h
# End Source File
# Begin Source File

SOURCE=.\include\RayView.h
# End Source File
# Begin Source File

SOURCE=.\include\ReflectiveMatDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\ReflectiveMaterial.h
# End Source File
# Begin Source File

SOURCE=.\include\Scene.h
# End Source File
# Begin Source File

SOURCE=.\include\SceneTreeView.h
# End Source File
# Begin Source File

SOURCE=.\include\Semaphore.h
# End Source File
# Begin Source File

SOURCE=.\include\SetColorDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\SettingsDialog.h
# End Source File
# Begin Source File

SOURCE=.\include\SpecMatDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\SpecularMaterial.h
# End Source File
# Begin Source File

SOURCE=.\include\Sphere.h
# End Source File
# Begin Source File

SOURCE=.\include\SpherePropertiesDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\include\Stream.h
# End Source File
# Begin Source File

SOURCE=.\include\TaskMaster.h
# End Source File
# Begin Source File

SOURCE=.\include\TaskSlave.h
# End Source File
# Begin Source File

SOURCE=.\include\ThreadTaskMaster.h
# End Source File
# Begin Source File

SOURCE=.\include\ThreadTaskSlave.h
# End Source File
# Begin Source File

SOURCE=.\include\Transform.h
# End Source File
# Begin Source File

SOURCE=.\include\TransparentMatDlg.h
# End Source File
# Begin Source File

SOURCE=.\include\TransparentMaterial.h
# End Source File
# Begin Source File

SOURCE=.\include\Triangle.h
# End Source File
# Begin Source File

SOURCE=.\include\Vector.h
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
