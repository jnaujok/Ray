# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Ray - Win32 Release
!MESSAGE No configuration specified.  Defaulting to Ray - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Ray - Win32 Release" && "$(CFG)" != "Ray - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ray.mak" CFG="Ray - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ray - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Ray - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "Ray - Win32 Debug"
MTL=mktyplib.exe
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "Ray - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\Ray.exe" "$(OUTDIR)\Ray.pch" "$(OUTDIR)\Ray.hlp"

CLEAN : 
	-@erase "$(INTDIR)\AddMaterialsDlg.obj"
	-@erase "$(INTDIR)\AddPlaneDlg.obj"
	-@erase "$(INTDIR)\AddPtLightDlg.obj"
	-@erase "$(INTDIR)\AddSphereDlg.obj"
	-@erase "$(INTDIR)\ambientmaterial.obj"
	-@erase "$(INTDIR)\camera.obj"
	-@erase "$(INTDIR)\CameraPropertiesDlg.obj"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\colorarray.obj"
	-@erase "$(INTDIR)\diffusematerial.obj"
	-@erase "$(INTDIR)\DiffuseMaterialPropDlg.obj"
	-@erase "$(INTDIR)\filestream.obj"
	-@erase "$(INTDIR)\imagefile.obj"
	-@erase "$(INTDIR)\InfluenceDlg.obj"
	-@erase "$(INTDIR)\light.obj"
	-@erase "$(INTDIR)\lightarray.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\material.obj"
	-@erase "$(INTDIR)\materiallist.obj"
	-@erase "$(INTDIR)\MaterialsDlg.obj"
	-@erase "$(INTDIR)\Matrix.obj"
	-@erase "$(INTDIR)\objects.obj"
	-@erase "$(INTDIR)\objectsarray.obj"
	-@erase "$(INTDIR)\pipestream.obj"
	-@erase "$(INTDIR)\plane.obj"
	-@erase "$(INTDIR)\PlanePropertiesDlg.obj"
	-@erase "$(INTDIR)\PointLightPropertiesDlg.obj"
	-@erase "$(INTDIR)\ptlight.obj"
	-@erase "$(INTDIR)\pvmstream.obj"
	-@erase "$(INTDIR)\pvmtaskmaster.obj"
	-@erase "$(INTDIR)\pvmtaskslave.obj"
	-@erase "$(INTDIR)\Ray.hlp"
	-@erase "$(INTDIR)\Ray.obj"
	-@erase "$(INTDIR)\Ray.pch"
	-@erase "$(INTDIR)\Ray.res"
	-@erase "$(INTDIR)\RayArray.obj"
	-@erase "$(INTDIR)\RayDoc.obj"
	-@erase "$(INTDIR)\rays.obj"
	-@erase "$(INTDIR)\raysettings.obj"
	-@erase "$(INTDIR)\RayView.obj"
	-@erase "$(INTDIR)\ReflectiveMatDlg.obj"
	-@erase "$(INTDIR)\reflectivematerial.obj"
	-@erase "$(INTDIR)\scene.obj"
	-@erase "$(INTDIR)\scenetreeview.obj"
	-@erase "$(INTDIR)\semaphore.obj"
	-@erase "$(INTDIR)\SetColorDlg.obj"
	-@erase "$(INTDIR)\SettingsDialog.obj"
	-@erase "$(INTDIR)\SpecMatDlg.obj"
	-@erase "$(INTDIR)\specularmaterial.obj"
	-@erase "$(INTDIR)\sphere.obj"
	-@erase "$(INTDIR)\SpherePropertiesDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\taskmaster.obj"
	-@erase "$(INTDIR)\taskslave.obj"
	-@erase "$(INTDIR)\threadtaskmaster.obj"
	-@erase "$(INTDIR)\threadtaskslave.obj"
	-@erase "$(INTDIR)\Transfrm.obj"
	-@erase "$(INTDIR)\TransparentMatDlg.obj"
	-@erase "$(INTDIR)\transparentmaterial.obj"
	-@erase "$(INTDIR)\triangle.obj"
	-@erase "$(INTDIR)\vector.obj"
	-@erase "$(OUTDIR)\Ray.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /I "pvm3\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
CPP_PROJ=/nologo /MT /W3 /GX /I "pvm3\include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_MBCS" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Ray.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Ray.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcd.lib"
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Ray.pdb" /machine:I386 /nodefaultlib:"libcd.lib"\
 /out:"$(OUTDIR)/Ray.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AddMaterialsDlg.obj" \
	"$(INTDIR)\AddPlaneDlg.obj" \
	"$(INTDIR)\AddPtLightDlg.obj" \
	"$(INTDIR)\AddSphereDlg.obj" \
	"$(INTDIR)\ambientmaterial.obj" \
	"$(INTDIR)\camera.obj" \
	"$(INTDIR)\CameraPropertiesDlg.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\colorarray.obj" \
	"$(INTDIR)\diffusematerial.obj" \
	"$(INTDIR)\DiffuseMaterialPropDlg.obj" \
	"$(INTDIR)\filestream.obj" \
	"$(INTDIR)\imagefile.obj" \
	"$(INTDIR)\InfluenceDlg.obj" \
	"$(INTDIR)\light.obj" \
	"$(INTDIR)\lightarray.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\material.obj" \
	"$(INTDIR)\materiallist.obj" \
	"$(INTDIR)\MaterialsDlg.obj" \
	"$(INTDIR)\Matrix.obj" \
	"$(INTDIR)\objects.obj" \
	"$(INTDIR)\objectsarray.obj" \
	"$(INTDIR)\pipestream.obj" \
	"$(INTDIR)\plane.obj" \
	"$(INTDIR)\PlanePropertiesDlg.obj" \
	"$(INTDIR)\PointLightPropertiesDlg.obj" \
	"$(INTDIR)\ptlight.obj" \
	"$(INTDIR)\pvmstream.obj" \
	"$(INTDIR)\pvmtaskmaster.obj" \
	"$(INTDIR)\pvmtaskslave.obj" \
	"$(INTDIR)\Ray.obj" \
	"$(INTDIR)\Ray.res" \
	"$(INTDIR)\RayArray.obj" \
	"$(INTDIR)\RayDoc.obj" \
	"$(INTDIR)\rays.obj" \
	"$(INTDIR)\raysettings.obj" \
	"$(INTDIR)\RayView.obj" \
	"$(INTDIR)\ReflectiveMatDlg.obj" \
	"$(INTDIR)\reflectivematerial.obj" \
	"$(INTDIR)\scene.obj" \
	"$(INTDIR)\scenetreeview.obj" \
	"$(INTDIR)\semaphore.obj" \
	"$(INTDIR)\SetColorDlg.obj" \
	"$(INTDIR)\SettingsDialog.obj" \
	"$(INTDIR)\SpecMatDlg.obj" \
	"$(INTDIR)\specularmaterial.obj" \
	"$(INTDIR)\sphere.obj" \
	"$(INTDIR)\SpherePropertiesDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\taskmaster.obj" \
	"$(INTDIR)\taskslave.obj" \
	"$(INTDIR)\threadtaskmaster.obj" \
	"$(INTDIR)\threadtaskslave.obj" \
	"$(INTDIR)\Transfrm.obj" \
	"$(INTDIR)\TransparentMatDlg.obj" \
	"$(INTDIR)\transparentmaterial.obj" \
	"$(INTDIR)\triangle.obj" \
	"$(INTDIR)\vector.obj"

"$(OUTDIR)\Ray.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\Ray.exe" "$(OUTDIR)\Ray.pch" "$(OUTDIR)\Ray.bsc"\
 "$(OUTDIR)\Ray.hlp"

CLEAN : 
	-@erase "$(INTDIR)\AddMaterialsDlg.obj"
	-@erase "$(INTDIR)\AddMaterialsDlg.sbr"
	-@erase "$(INTDIR)\AddPlaneDlg.obj"
	-@erase "$(INTDIR)\AddPlaneDlg.sbr"
	-@erase "$(INTDIR)\AddPtLightDlg.obj"
	-@erase "$(INTDIR)\AddPtLightDlg.sbr"
	-@erase "$(INTDIR)\AddSphereDlg.obj"
	-@erase "$(INTDIR)\AddSphereDlg.sbr"
	-@erase "$(INTDIR)\ambientmaterial.obj"
	-@erase "$(INTDIR)\ambientmaterial.sbr"
	-@erase "$(INTDIR)\camera.obj"
	-@erase "$(INTDIR)\camera.sbr"
	-@erase "$(INTDIR)\CameraPropertiesDlg.obj"
	-@erase "$(INTDIR)\CameraPropertiesDlg.sbr"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\ChildFrm.sbr"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\color.sbr"
	-@erase "$(INTDIR)\colorarray.obj"
	-@erase "$(INTDIR)\colorarray.sbr"
	-@erase "$(INTDIR)\diffusematerial.obj"
	-@erase "$(INTDIR)\diffusematerial.sbr"
	-@erase "$(INTDIR)\DiffuseMaterialPropDlg.obj"
	-@erase "$(INTDIR)\DiffuseMaterialPropDlg.sbr"
	-@erase "$(INTDIR)\filestream.obj"
	-@erase "$(INTDIR)\filestream.sbr"
	-@erase "$(INTDIR)\imagefile.obj"
	-@erase "$(INTDIR)\imagefile.sbr"
	-@erase "$(INTDIR)\InfluenceDlg.obj"
	-@erase "$(INTDIR)\InfluenceDlg.sbr"
	-@erase "$(INTDIR)\light.obj"
	-@erase "$(INTDIR)\light.sbr"
	-@erase "$(INTDIR)\lightarray.obj"
	-@erase "$(INTDIR)\lightarray.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\material.obj"
	-@erase "$(INTDIR)\material.sbr"
	-@erase "$(INTDIR)\materiallist.obj"
	-@erase "$(INTDIR)\materiallist.sbr"
	-@erase "$(INTDIR)\MaterialsDlg.obj"
	-@erase "$(INTDIR)\MaterialsDlg.sbr"
	-@erase "$(INTDIR)\Matrix.obj"
	-@erase "$(INTDIR)\Matrix.sbr"
	-@erase "$(INTDIR)\objects.obj"
	-@erase "$(INTDIR)\objects.sbr"
	-@erase "$(INTDIR)\objectsarray.obj"
	-@erase "$(INTDIR)\objectsarray.sbr"
	-@erase "$(INTDIR)\pipestream.obj"
	-@erase "$(INTDIR)\pipestream.sbr"
	-@erase "$(INTDIR)\plane.obj"
	-@erase "$(INTDIR)\plane.sbr"
	-@erase "$(INTDIR)\PlanePropertiesDlg.obj"
	-@erase "$(INTDIR)\PlanePropertiesDlg.sbr"
	-@erase "$(INTDIR)\PointLightPropertiesDlg.obj"
	-@erase "$(INTDIR)\PointLightPropertiesDlg.sbr"
	-@erase "$(INTDIR)\ptlight.obj"
	-@erase "$(INTDIR)\ptlight.sbr"
	-@erase "$(INTDIR)\pvmstream.obj"
	-@erase "$(INTDIR)\pvmstream.sbr"
	-@erase "$(INTDIR)\pvmtaskmaster.obj"
	-@erase "$(INTDIR)\pvmtaskmaster.sbr"
	-@erase "$(INTDIR)\pvmtaskslave.obj"
	-@erase "$(INTDIR)\pvmtaskslave.sbr"
	-@erase "$(INTDIR)\Ray.hlp"
	-@erase "$(INTDIR)\Ray.obj"
	-@erase "$(INTDIR)\Ray.pch"
	-@erase "$(INTDIR)\Ray.res"
	-@erase "$(INTDIR)\Ray.sbr"
	-@erase "$(INTDIR)\RayArray.obj"
	-@erase "$(INTDIR)\RayArray.sbr"
	-@erase "$(INTDIR)\RayDoc.obj"
	-@erase "$(INTDIR)\RayDoc.sbr"
	-@erase "$(INTDIR)\rays.obj"
	-@erase "$(INTDIR)\rays.sbr"
	-@erase "$(INTDIR)\raysettings.obj"
	-@erase "$(INTDIR)\raysettings.sbr"
	-@erase "$(INTDIR)\RayView.obj"
	-@erase "$(INTDIR)\RayView.sbr"
	-@erase "$(INTDIR)\ReflectiveMatDlg.obj"
	-@erase "$(INTDIR)\ReflectiveMatDlg.sbr"
	-@erase "$(INTDIR)\reflectivematerial.obj"
	-@erase "$(INTDIR)\reflectivematerial.sbr"
	-@erase "$(INTDIR)\scene.obj"
	-@erase "$(INTDIR)\scene.sbr"
	-@erase "$(INTDIR)\scenetreeview.obj"
	-@erase "$(INTDIR)\scenetreeview.sbr"
	-@erase "$(INTDIR)\semaphore.obj"
	-@erase "$(INTDIR)\semaphore.sbr"
	-@erase "$(INTDIR)\SetColorDlg.obj"
	-@erase "$(INTDIR)\SetColorDlg.sbr"
	-@erase "$(INTDIR)\SettingsDialog.obj"
	-@erase "$(INTDIR)\SettingsDialog.sbr"
	-@erase "$(INTDIR)\SpecMatDlg.obj"
	-@erase "$(INTDIR)\SpecMatDlg.sbr"
	-@erase "$(INTDIR)\specularmaterial.obj"
	-@erase "$(INTDIR)\specularmaterial.sbr"
	-@erase "$(INTDIR)\sphere.obj"
	-@erase "$(INTDIR)\sphere.sbr"
	-@erase "$(INTDIR)\SpherePropertiesDlg.obj"
	-@erase "$(INTDIR)\SpherePropertiesDlg.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\taskmaster.obj"
	-@erase "$(INTDIR)\taskmaster.sbr"
	-@erase "$(INTDIR)\taskslave.obj"
	-@erase "$(INTDIR)\taskslave.sbr"
	-@erase "$(INTDIR)\threadtaskmaster.obj"
	-@erase "$(INTDIR)\threadtaskmaster.sbr"
	-@erase "$(INTDIR)\threadtaskslave.obj"
	-@erase "$(INTDIR)\threadtaskslave.sbr"
	-@erase "$(INTDIR)\Transfrm.obj"
	-@erase "$(INTDIR)\Transfrm.sbr"
	-@erase "$(INTDIR)\TransparentMatDlg.obj"
	-@erase "$(INTDIR)\TransparentMatDlg.sbr"
	-@erase "$(INTDIR)\transparentmaterial.obj"
	-@erase "$(INTDIR)\transparentmaterial.sbr"
	-@erase "$(INTDIR)\triangle.obj"
	-@erase "$(INTDIR)\triangle.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\vector.obj"
	-@erase "$(INTDIR)\vector.sbr"
	-@erase "$(OUTDIR)\Ray.bsc"
	-@erase "$(OUTDIR)\Ray.exe"
	-@erase "$(OUTDIR)\Ray.ilk"
	-@erase "$(OUTDIR)\Ray.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "pvm3\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /c
# SUBTRACT CPP /YX /Yc /Yu
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "pvm3\include" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)/" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Ray.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Ray.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AddMaterialsDlg.sbr" \
	"$(INTDIR)\AddPlaneDlg.sbr" \
	"$(INTDIR)\AddPtLightDlg.sbr" \
	"$(INTDIR)\AddSphereDlg.sbr" \
	"$(INTDIR)\ambientmaterial.sbr" \
	"$(INTDIR)\camera.sbr" \
	"$(INTDIR)\CameraPropertiesDlg.sbr" \
	"$(INTDIR)\ChildFrm.sbr" \
	"$(INTDIR)\color.sbr" \
	"$(INTDIR)\colorarray.sbr" \
	"$(INTDIR)\diffusematerial.sbr" \
	"$(INTDIR)\DiffuseMaterialPropDlg.sbr" \
	"$(INTDIR)\filestream.sbr" \
	"$(INTDIR)\imagefile.sbr" \
	"$(INTDIR)\InfluenceDlg.sbr" \
	"$(INTDIR)\light.sbr" \
	"$(INTDIR)\lightarray.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\material.sbr" \
	"$(INTDIR)\materiallist.sbr" \
	"$(INTDIR)\MaterialsDlg.sbr" \
	"$(INTDIR)\Matrix.sbr" \
	"$(INTDIR)\objects.sbr" \
	"$(INTDIR)\objectsarray.sbr" \
	"$(INTDIR)\pipestream.sbr" \
	"$(INTDIR)\plane.sbr" \
	"$(INTDIR)\PlanePropertiesDlg.sbr" \
	"$(INTDIR)\PointLightPropertiesDlg.sbr" \
	"$(INTDIR)\ptlight.sbr" \
	"$(INTDIR)\pvmstream.sbr" \
	"$(INTDIR)\pvmtaskmaster.sbr" \
	"$(INTDIR)\pvmtaskslave.sbr" \
	"$(INTDIR)\Ray.sbr" \
	"$(INTDIR)\RayArray.sbr" \
	"$(INTDIR)\RayDoc.sbr" \
	"$(INTDIR)\rays.sbr" \
	"$(INTDIR)\raysettings.sbr" \
	"$(INTDIR)\RayView.sbr" \
	"$(INTDIR)\ReflectiveMatDlg.sbr" \
	"$(INTDIR)\reflectivematerial.sbr" \
	"$(INTDIR)\scene.sbr" \
	"$(INTDIR)\scenetreeview.sbr" \
	"$(INTDIR)\semaphore.sbr" \
	"$(INTDIR)\SetColorDlg.sbr" \
	"$(INTDIR)\SettingsDialog.sbr" \
	"$(INTDIR)\SpecMatDlg.sbr" \
	"$(INTDIR)\specularmaterial.sbr" \
	"$(INTDIR)\sphere.sbr" \
	"$(INTDIR)\SpherePropertiesDlg.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\taskmaster.sbr" \
	"$(INTDIR)\taskslave.sbr" \
	"$(INTDIR)\threadtaskmaster.sbr" \
	"$(INTDIR)\threadtaskslave.sbr" \
	"$(INTDIR)\Transfrm.sbr" \
	"$(INTDIR)\TransparentMatDlg.sbr" \
	"$(INTDIR)\transparentmaterial.sbr" \
	"$(INTDIR)\triangle.sbr" \
	"$(INTDIR)\vector.sbr"

"$(OUTDIR)\Ray.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib"
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/Ray.pdb" /debug /machine:I386 /nodefaultlib:"libcd.lib"\
 /out:"$(OUTDIR)/Ray.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AddMaterialsDlg.obj" \
	"$(INTDIR)\AddPlaneDlg.obj" \
	"$(INTDIR)\AddPtLightDlg.obj" \
	"$(INTDIR)\AddSphereDlg.obj" \
	"$(INTDIR)\ambientmaterial.obj" \
	"$(INTDIR)\camera.obj" \
	"$(INTDIR)\CameraPropertiesDlg.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\colorarray.obj" \
	"$(INTDIR)\diffusematerial.obj" \
	"$(INTDIR)\DiffuseMaterialPropDlg.obj" \
	"$(INTDIR)\filestream.obj" \
	"$(INTDIR)\imagefile.obj" \
	"$(INTDIR)\InfluenceDlg.obj" \
	"$(INTDIR)\light.obj" \
	"$(INTDIR)\lightarray.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\material.obj" \
	"$(INTDIR)\materiallist.obj" \
	"$(INTDIR)\MaterialsDlg.obj" \
	"$(INTDIR)\Matrix.obj" \
	"$(INTDIR)\objects.obj" \
	"$(INTDIR)\objectsarray.obj" \
	"$(INTDIR)\pipestream.obj" \
	"$(INTDIR)\plane.obj" \
	"$(INTDIR)\PlanePropertiesDlg.obj" \
	"$(INTDIR)\PointLightPropertiesDlg.obj" \
	"$(INTDIR)\ptlight.obj" \
	"$(INTDIR)\pvmstream.obj" \
	"$(INTDIR)\pvmtaskmaster.obj" \
	"$(INTDIR)\pvmtaskslave.obj" \
	"$(INTDIR)\Ray.obj" \
	"$(INTDIR)\Ray.res" \
	"$(INTDIR)\RayArray.obj" \
	"$(INTDIR)\RayDoc.obj" \
	"$(INTDIR)\rays.obj" \
	"$(INTDIR)\raysettings.obj" \
	"$(INTDIR)\RayView.obj" \
	"$(INTDIR)\ReflectiveMatDlg.obj" \
	"$(INTDIR)\reflectivematerial.obj" \
	"$(INTDIR)\scene.obj" \
	"$(INTDIR)\scenetreeview.obj" \
	"$(INTDIR)\semaphore.obj" \
	"$(INTDIR)\SetColorDlg.obj" \
	"$(INTDIR)\SettingsDialog.obj" \
	"$(INTDIR)\SpecMatDlg.obj" \
	"$(INTDIR)\specularmaterial.obj" \
	"$(INTDIR)\sphere.obj" \
	"$(INTDIR)\SpherePropertiesDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\taskmaster.obj" \
	"$(INTDIR)\taskslave.obj" \
	"$(INTDIR)\threadtaskmaster.obj" \
	"$(INTDIR)\threadtaskslave.obj" \
	"$(INTDIR)\Transfrm.obj" \
	"$(INTDIR)\TransparentMatDlg.obj" \
	"$(INTDIR)\transparentmaterial.obj" \
	"$(INTDIR)\triangle.obj" \
	"$(INTDIR)\vector.obj"

"$(OUTDIR)\Ray.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "Ray - Win32 Release"
# Name "Ray - Win32 Debug"

!IF  "$(CFG)" == "Ray - Win32 Release"

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "Ray - Win32 Release"

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Ray.cpp
DEP_CPP_RAY_C=\
	".\camera.h"\
	".\ChildFrm.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\MainFrm.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\RayView.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\Ray.obj" : $(SOURCE) $(DEP_CPP_RAY_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\Ray.obj" : $(SOURCE) $(DEP_CPP_RAY_C) "$(INTDIR)"

"$(INTDIR)\Ray.sbr" : $(SOURCE) $(DEP_CPP_RAY_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /I "pvm3\include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)/Ray.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Ray.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I "pvm3\include" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Ray.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Ray.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp
DEP_CPP_MAINF=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\MainFrm.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ChildFrm.cpp
DEP_CPP_CHILD=\
	".\camera.h"\
	".\ChildFrm.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\RayView.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\ChildFrm.obj" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\ChildFrm.obj" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"

"$(INTDIR)\ChildFrm.sbr" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RayDoc.cpp
DEP_CPP_RAYDO=\
	".\AddPlaneDlg.h"\
	".\AddPtLightDlg.h"\
	".\AddSphereDlg.h"\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\filestream.h"\
	".\globals.h"\
	".\ImageFile.h"\
	".\light.h"\
	".\lightarray.h"\
	".\MainFrm.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pipestream.h"\
	".\plane.h"\
	".\ptlight.h"\
	".\pvmtaskmaster.h"\
	".\Ray.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\semaphore.h"\
	".\sphere.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskmaster.h"\
	".\Threadtaskmaster.h"\
	".\triangle.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\RayDoc.obj" : $(SOURCE) $(DEP_CPP_RAYDO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\RayDoc.obj" : $(SOURCE) $(DEP_CPP_RAYDO) "$(INTDIR)"

"$(INTDIR)\RayDoc.sbr" : $(SOURCE) $(DEP_CPP_RAYDO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RayView.cpp
DEP_CPP_RAYVI=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\MainFrm.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\RayView.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\RayView.obj" : $(SOURCE) $(DEP_CPP_RAYVI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\RayView.obj" : $(SOURCE) $(DEP_CPP_RAYVI) "$(INTDIR)"

"$(INTDIR)\RayView.sbr" : $(SOURCE) $(DEP_CPP_RAYVI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Ray.rc
DEP_RSC_RAY_R=\
	".\res\about_bi.bmp"\
	".\res\bitmap1.bmp"\
	".\res\Ray.ico"\
	".\res\Ray.rc2"\
	".\res\RayDoc.ico"\
	".\res\scene_tr.bmp"\
	".\res\small_ic.bmp"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\Ray.res" : $(SOURCE) $(DEP_RSC_RAY_R) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\hlp\Ray.hpj

!IF  "$(CFG)" == "Ray - Win32 Release"

# Begin Custom Build - Making help file...
OutDir=.\Release
ProjDir=.
TargetName=Ray
InputPath=.\hlp\Ray.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   "$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"

# Begin Custom Build - Making help file...
OutDir=.\Debug
ProjDir=.
TargetName=Ray
InputPath=.\hlp\Ray.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   "$(ProjDir)\makehelp.bat"

# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\scene.cpp
DEP_CPP_SCENE=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\scene.obj" : $(SOURCE) $(DEP_CPP_SCENE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\scene.obj" : $(SOURCE) $(DEP_CPP_SCENE) "$(INTDIR)"

"$(INTDIR)\scene.sbr" : $(SOURCE) $(DEP_CPP_SCENE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vector.cpp
DEP_CPP_VECTO=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\vector.obj" : $(SOURCE) $(DEP_CPP_VECTO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\vector.obj" : $(SOURCE) $(DEP_CPP_VECTO) "$(INTDIR)"

"$(INTDIR)\vector.sbr" : $(SOURCE) $(DEP_CPP_VECTO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\color.cpp
DEP_CPP_COLOR=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\color.obj" : $(SOURCE) $(DEP_CPP_COLOR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\color.obj" : $(SOURCE) $(DEP_CPP_COLOR) "$(INTDIR)"

"$(INTDIR)\color.sbr" : $(SOURCE) $(DEP_CPP_COLOR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Matrix.cpp
DEP_CPP_MATRI=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\Matrix.obj" : $(SOURCE) $(DEP_CPP_MATRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\Matrix.obj" : $(SOURCE) $(DEP_CPP_MATRI) "$(INTDIR)"

"$(INTDIR)\Matrix.sbr" : $(SOURCE) $(DEP_CPP_MATRI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Transfrm.cpp
DEP_CPP_TRANS=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\Transfrm.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\Transfrm.obj" : $(SOURCE) $(DEP_CPP_TRANS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\Transfrm.obj" : $(SOURCE) $(DEP_CPP_TRANS) "$(INTDIR)"

"$(INTDIR)\Transfrm.sbr" : $(SOURCE) $(DEP_CPP_TRANS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\light.cpp
DEP_CPP_LIGHT=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\light.obj" : $(SOURCE) $(DEP_CPP_LIGHT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\light.obj" : $(SOURCE) $(DEP_CPP_LIGHT) "$(INTDIR)"

"$(INTDIR)\light.sbr" : $(SOURCE) $(DEP_CPP_LIGHT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ptlight.cpp
DEP_CPP_PTLIG=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\PointLightPropertiesDlg.h"\
	".\ptlight.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\ptlight.obj" : $(SOURCE) $(DEP_CPP_PTLIG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\ptlight.obj" : $(SOURCE) $(DEP_CPP_PTLIG) "$(INTDIR)"

"$(INTDIR)\ptlight.sbr" : $(SOURCE) $(DEP_CPP_PTLIG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\objects.cpp
DEP_CPP_OBJEC=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\MaterialsDlg.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\objects.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\objects.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"

"$(INTDIR)\objects.sbr" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\plane.cpp
DEP_CPP_PLANE=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\plane.h"\
	".\PlanePropertiesDlg.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\plane.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\plane.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"

"$(INTDIR)\plane.sbr" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sphere.cpp
DEP_CPP_SPHER=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\sphere.h"\
	".\SpherePropertiesDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\sphere.obj" : $(SOURCE) $(DEP_CPP_SPHER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\sphere.obj" : $(SOURCE) $(DEP_CPP_SPHER) "$(INTDIR)"

"$(INTDIR)\sphere.sbr" : $(SOURCE) $(DEP_CPP_SPHER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AddPtLightDlg.cpp
DEP_CPP_ADDPT=\
	".\AddPtLightDlg.h"\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\AddPtLightDlg.obj" : $(SOURCE) $(DEP_CPP_ADDPT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\AddPtLightDlg.obj" : $(SOURCE) $(DEP_CPP_ADDPT) "$(INTDIR)"

"$(INTDIR)\AddPtLightDlg.sbr" : $(SOURCE) $(DEP_CPP_ADDPT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AddSphereDlg.cpp
DEP_CPP_ADDSP=\
	".\AddSphereDlg.h"\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\AddSphereDlg.obj" : $(SOURCE) $(DEP_CPP_ADDSP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\AddSphereDlg.obj" : $(SOURCE) $(DEP_CPP_ADDSP) "$(INTDIR)"

"$(INTDIR)\AddSphereDlg.sbr" : $(SOURCE) $(DEP_CPP_ADDSP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AddPlaneDlg.cpp
DEP_CPP_ADDPL=\
	".\AddPlaneDlg.h"\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\AddPlaneDlg.obj" : $(SOURCE) $(DEP_CPP_ADDPL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\AddPlaneDlg.obj" : $(SOURCE) $(DEP_CPP_ADDPL) "$(INTDIR)"

"$(INTDIR)\AddPlaneDlg.sbr" : $(SOURCE) $(DEP_CPP_ADDPL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\camera.cpp
DEP_CPP_CAMER=\
	".\camera.h"\
	".\CameraPropertiesDlg.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\camera.obj" : $(SOURCE) $(DEP_CPP_CAMER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\camera.obj" : $(SOURCE) $(DEP_CPP_CAMER) "$(INTDIR)"

"$(INTDIR)\camera.sbr" : $(SOURCE) $(DEP_CPP_CAMER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rays.cpp
DEP_CPP_RAYS_=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\Transfrm.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\rays.obj" : $(SOURCE) $(DEP_CPP_RAYS_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\rays.obj" : $(SOURCE) $(DEP_CPP_RAYS_) "$(INTDIR)"

"$(INTDIR)\rays.sbr" : $(SOURCE) $(DEP_CPP_RAYS_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SetColorDlg.cpp
DEP_CPP_SETCO=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\SetColorDlg.obj" : $(SOURCE) $(DEP_CPP_SETCO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\SetColorDlg.obj" : $(SOURCE) $(DEP_CPP_SETCO) "$(INTDIR)"

"$(INTDIR)\SetColorDlg.sbr" : $(SOURCE) $(DEP_CPP_SETCO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PlanePropertiesDlg.cpp
DEP_CPP_PLANEP=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\PlanePropertiesDlg.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\PlanePropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_PLANEP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\PlanePropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_PLANEP) "$(INTDIR)"

"$(INTDIR)\PlanePropertiesDlg.sbr" : $(SOURCE) $(DEP_CPP_PLANEP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SpherePropertiesDlg.cpp
DEP_CPP_SPHERE=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\SpherePropertiesDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\SpherePropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_SPHERE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\SpherePropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_SPHERE) "$(INTDIR)"

"$(INTDIR)\SpherePropertiesDlg.sbr" : $(SOURCE) $(DEP_CPP_SPHERE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\filestream.cpp
DEP_CPP_FILES=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\filestream.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\filestream.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\filestream.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"

"$(INTDIR)\filestream.sbr" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\raysettings.cpp
DEP_CPP_RAYSE=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SettingsDialog.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\raysettings.obj" : $(SOURCE) $(DEP_CPP_RAYSE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\raysettings.obj" : $(SOURCE) $(DEP_CPP_RAYSE) "$(INTDIR)"

"$(INTDIR)\raysettings.sbr" : $(SOURCE) $(DEP_CPP_RAYSE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CameraPropertiesDlg.cpp
DEP_CPP_CAMERA=\
	".\camera.h"\
	".\CameraPropertiesDlg.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\CameraPropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_CAMERA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\CameraPropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_CAMERA) "$(INTDIR)"

"$(INTDIR)\CameraPropertiesDlg.sbr" : $(SOURCE) $(DEP_CPP_CAMERA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PointLightPropertiesDlg.cpp
DEP_CPP_POINT=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\PointLightPropertiesDlg.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\PointLightPropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_POINT)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\PointLightPropertiesDlg.obj" : $(SOURCE) $(DEP_CPP_POINT)\
 "$(INTDIR)"

"$(INTDIR)\PointLightPropertiesDlg.sbr" : $(SOURCE) $(DEP_CPP_POINT)\
 "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lightarray.cpp
DEP_CPP_LIGHTA=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\lightarray.obj" : $(SOURCE) $(DEP_CPP_LIGHTA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\lightarray.obj" : $(SOURCE) $(DEP_CPP_LIGHTA) "$(INTDIR)"

"$(INTDIR)\lightarray.sbr" : $(SOURCE) $(DEP_CPP_LIGHTA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\objectsarray.cpp
DEP_CPP_OBJECT=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\objectsarray.obj" : $(SOURCE) $(DEP_CPP_OBJECT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\objectsarray.obj" : $(SOURCE) $(DEP_CPP_OBJECT) "$(INTDIR)"

"$(INTDIR)\objectsarray.sbr" : $(SOURCE) $(DEP_CPP_OBJECT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\material.cpp
DEP_CPP_MATER=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\material.obj" : $(SOURCE) $(DEP_CPP_MATER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\material.obj" : $(SOURCE) $(DEP_CPP_MATER) "$(INTDIR)"

"$(INTDIR)\material.sbr" : $(SOURCE) $(DEP_CPP_MATER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\materiallist.cpp
DEP_CPP_MATERI=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\materiallist.obj" : $(SOURCE) $(DEP_CPP_MATERI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\materiallist.obj" : $(SOURCE) $(DEP_CPP_MATERI) "$(INTDIR)"

"$(INTDIR)\materiallist.sbr" : $(SOURCE) $(DEP_CPP_MATERI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diffusematerial.cpp
DEP_CPP_DIFFU=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\DiffuseMaterialPropDlg.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\diffusematerial.obj" : $(SOURCE) $(DEP_CPP_DIFFU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\diffusematerial.obj" : $(SOURCE) $(DEP_CPP_DIFFU) "$(INTDIR)"

"$(INTDIR)\diffusematerial.sbr" : $(SOURCE) $(DEP_CPP_DIFFU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\specularmaterial.cpp
DEP_CPP_SPECU=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SpecMatDlg.h"\
	".\SpecularMaterial.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\specularmaterial.obj" : $(SOURCE) $(DEP_CPP_SPECU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\specularmaterial.obj" : $(SOURCE) $(DEP_CPP_SPECU) "$(INTDIR)"

"$(INTDIR)\specularmaterial.sbr" : $(SOURCE) $(DEP_CPP_SPECU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\reflectivematerial.cpp
DEP_CPP_REFLE=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\ReflectiveMatDlg.h"\
	".\reflectivematerial.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\reflectivematerial.obj" : $(SOURCE) $(DEP_CPP_REFLE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\reflectivematerial.obj" : $(SOURCE) $(DEP_CPP_REFLE) "$(INTDIR)"

"$(INTDIR)\reflectivematerial.sbr" : $(SOURCE) $(DEP_CPP_REFLE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MaterialsDlg.cpp
DEP_CPP_MATERIA=\
	".\AddMaterialsDlg.h"\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\InfluenceDlg.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\MaterialsDlg.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\MaterialsDlg.obj" : $(SOURCE) $(DEP_CPP_MATERIA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\MaterialsDlg.obj" : $(SOURCE) $(DEP_CPP_MATERIA) "$(INTDIR)"

"$(INTDIR)\MaterialsDlg.sbr" : $(SOURCE) $(DEP_CPP_MATERIA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\scenetreeview.cpp
DEP_CPP_SCENET=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\MainFrm.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\scenetreeview.obj" : $(SOURCE) $(DEP_CPP_SCENET) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\scenetreeview.obj" : $(SOURCE) $(DEP_CPP_SCENET) "$(INTDIR)"

"$(INTDIR)\scenetreeview.sbr" : $(SOURCE) $(DEP_CPP_SCENET) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DiffuseMaterialPropDlg.cpp
DEP_CPP_DIFFUS=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\DiffuseMaterialPropDlg.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\DiffuseMaterialPropDlg.obj" : $(SOURCE) $(DEP_CPP_DIFFUS)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\DiffuseMaterialPropDlg.obj" : $(SOURCE) $(DEP_CPP_DIFFUS)\
 "$(INTDIR)"

"$(INTDIR)\DiffuseMaterialPropDlg.sbr" : $(SOURCE) $(DEP_CPP_DIFFUS)\
 "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SpecMatDlg.cpp
DEP_CPP_SPECM=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\SpecMatDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\SpecMatDlg.obj" : $(SOURCE) $(DEP_CPP_SPECM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\SpecMatDlg.obj" : $(SOURCE) $(DEP_CPP_SPECM) "$(INTDIR)"

"$(INTDIR)\SpecMatDlg.sbr" : $(SOURCE) $(DEP_CPP_SPECM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ReflectiveMatDlg.cpp
DEP_CPP_REFLEC=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\ReflectiveMatDlg.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\ReflectiveMatDlg.obj" : $(SOURCE) $(DEP_CPP_REFLEC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\ReflectiveMatDlg.obj" : $(SOURCE) $(DEP_CPP_REFLEC) "$(INTDIR)"

"$(INTDIR)\ReflectiveMatDlg.sbr" : $(SOURCE) $(DEP_CPP_REFLEC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\InfluenceDlg.cpp
DEP_CPP_INFLU=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\InfluenceDlg.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\InfluenceDlg.obj" : $(SOURCE) $(DEP_CPP_INFLU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\InfluenceDlg.obj" : $(SOURCE) $(DEP_CPP_INFLU) "$(INTDIR)"

"$(INTDIR)\InfluenceDlg.sbr" : $(SOURCE) $(DEP_CPP_INFLU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AddMaterialsDlg.cpp
DEP_CPP_ADDMA=\
	".\AddMaterialsDlg.h"\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\AddMaterialsDlg.obj" : $(SOURCE) $(DEP_CPP_ADDMA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\AddMaterialsDlg.obj" : $(SOURCE) $(DEP_CPP_ADDMA) "$(INTDIR)"

"$(INTDIR)\AddMaterialsDlg.sbr" : $(SOURCE) $(DEP_CPP_ADDMA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TransparentMatDlg.cpp
DEP_CPP_TRANSP=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\TransparentMatDlg.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\TransparentMatDlg.obj" : $(SOURCE) $(DEP_CPP_TRANSP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\TransparentMatDlg.obj" : $(SOURCE) $(DEP_CPP_TRANSP) "$(INTDIR)"

"$(INTDIR)\TransparentMatDlg.sbr" : $(SOURCE) $(DEP_CPP_TRANSP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\transparentmaterial.cpp
DEP_CPP_TRANSPA=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\TransparentMatDlg.h"\
	".\transparentmaterial.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\transparentmaterial.obj" : $(SOURCE) $(DEP_CPP_TRANSPA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\transparentmaterial.obj" : $(SOURCE) $(DEP_CPP_TRANSPA) "$(INTDIR)"

"$(INTDIR)\transparentmaterial.sbr" : $(SOURCE) $(DEP_CPP_TRANSPA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ambientmaterial.cpp
DEP_CPP_AMBIE=\
	".\ambientmaterial.h"\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SetColorDlg.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\ambientmaterial.obj" : $(SOURCE) $(DEP_CPP_AMBIE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\ambientmaterial.obj" : $(SOURCE) $(DEP_CPP_AMBIE) "$(INTDIR)"

"$(INTDIR)\ambientmaterial.sbr" : $(SOURCE) $(DEP_CPP_AMBIE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SettingsDialog.cpp
DEP_CPP_SETTI=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\Ray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\SettingsDialog.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\SettingsDialog.obj" : $(SOURCE) $(DEP_CPP_SETTI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\SettingsDialog.obj" : $(SOURCE) $(DEP_CPP_SETTI) "$(INTDIR)"

"$(INTDIR)\SettingsDialog.sbr" : $(SOURCE) $(DEP_CPP_SETTI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RayArray.cpp
DEP_CPP_RAYAR=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\RayArray.obj" : $(SOURCE) $(DEP_CPP_RAYAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\RayArray.obj" : $(SOURCE) $(DEP_CPP_RAYAR) "$(INTDIR)"

"$(INTDIR)\RayArray.sbr" : $(SOURCE) $(DEP_CPP_RAYAR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\triangle.cpp
DEP_CPP_TRIAN=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\triangle.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\triangle.obj" : $(SOURCE) $(DEP_CPP_TRIAN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\triangle.obj" : $(SOURCE) $(DEP_CPP_TRIAN) "$(INTDIR)"

"$(INTDIR)\triangle.sbr" : $(SOURCE) $(DEP_CPP_TRIAN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\taskmaster.cpp
DEP_CPP_TASKM=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\ImageFile.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskmaster.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\taskmaster.obj" : $(SOURCE) $(DEP_CPP_TASKM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\taskmaster.obj" : $(SOURCE) $(DEP_CPP_TASKM) "$(INTDIR)"

"$(INTDIR)\taskmaster.sbr" : $(SOURCE) $(DEP_CPP_TASKM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\threadtaskmaster.cpp
DEP_CPP_THREA=\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\ImageFile.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\msgtag.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pipestream.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\semaphore.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskmaster.h"\
	".\taskslave.h"\
	".\Threadtaskmaster.h"\
	".\ThreadTaskSlave.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\threadtaskmaster.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\threadtaskmaster.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"

"$(INTDIR)\threadtaskmaster.sbr" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pipestream.cpp
DEP_CPP_PIPES=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pipestream.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\pipestream.obj" : $(SOURCE) $(DEP_CPP_PIPES) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\pipestream.obj" : $(SOURCE) $(DEP_CPP_PIPES) "$(INTDIR)"

"$(INTDIR)\pipestream.sbr" : $(SOURCE) $(DEP_CPP_PIPES) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\threadtaskslave.cpp
DEP_CPP_THREAD=\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\msgtag.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pipestream.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\semaphore.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskslave.h"\
	".\ThreadTaskSlave.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\threadtaskslave.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\threadtaskslave.obj" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"

"$(INTDIR)\threadtaskslave.sbr" : $(SOURCE) $(DEP_CPP_THREAD) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\colorarray.cpp
DEP_CPP_COLORA=\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\colorarray.obj" : $(SOURCE) $(DEP_CPP_COLORA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\colorarray.obj" : $(SOURCE) $(DEP_CPP_COLORA) "$(INTDIR)"

"$(INTDIR)\colorarray.sbr" : $(SOURCE) $(DEP_CPP_COLORA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\taskslave.cpp
DEP_CPP_TASKS=\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskslave.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\taskslave.obj" : $(SOURCE) $(DEP_CPP_TASKS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\taskslave.obj" : $(SOURCE) $(DEP_CPP_TASKS) "$(INTDIR)"

"$(INTDIR)\taskslave.sbr" : $(SOURCE) $(DEP_CPP_TASKS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\imagefile.cpp
DEP_CPP_IMAGE=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\ImageFile.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\imagefile.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\imagefile.obj" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"

"$(INTDIR)\imagefile.sbr" : $(SOURCE) $(DEP_CPP_IMAGE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\semaphore.cpp
DEP_CPP_SEMAP=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\semaphore.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\semaphore.obj" : $(SOURCE) $(DEP_CPP_SEMAP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\semaphore.obj" : $(SOURCE) $(DEP_CPP_SEMAP) "$(INTDIR)"

"$(INTDIR)\semaphore.sbr" : $(SOURCE) $(DEP_CPP_SEMAP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pvmtaskslave.cpp
DEP_CPP_PVMTA=\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\msgtag.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pvm3.h"\
	".\pvmstream.h"\
	".\pvmTaskSlave.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskslave.h"\
	".\vector.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\pvmtaskslave.obj" : $(SOURCE) $(DEP_CPP_PVMTA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\pvmtaskslave.obj" : $(SOURCE) $(DEP_CPP_PVMTA) "$(INTDIR)"

"$(INTDIR)\pvmtaskslave.sbr" : $(SOURCE) $(DEP_CPP_PVMTA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pvmtaskmaster.cpp
DEP_CPP_PVMTAS=\
	".\camera.h"\
	".\color.h"\
	".\colorarray.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\ImageFile.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\msgtag.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pvm3.h"\
	".\pvmstream.h"\
	".\pvmtaskmaster.h"\
	".\RayArray.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\taskmaster.h"\
	".\vector.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\pvmtaskmaster.obj" : $(SOURCE) $(DEP_CPP_PVMTAS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\pvmtaskmaster.obj" : $(SOURCE) $(DEP_CPP_PVMTAS) "$(INTDIR)"

"$(INTDIR)\pvmtaskmaster.sbr" : $(SOURCE) $(DEP_CPP_PVMTAS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pvmstream.cpp
DEP_CPP_PVMST=\
	".\camera.h"\
	".\color.h"\
	".\diffusematerial.h"\
	".\globals.h"\
	".\light.h"\
	".\lightarray.h"\
	".\material.h"\
	".\materiallist.h"\
	".\Matrix.h"\
	".\objects.h"\
	".\objectsarray.h"\
	".\pvm3.h"\
	".\pvmstream.h"\
	".\RayDoc.h"\
	".\RayEngine\cstring.h"\
	".\rays.h"\
	".\RaySettings.h"\
	".\scene.h"\
	".\scenetreeview.h"\
	".\StdAfx.h"\
	".\stream.h"\
	".\vector.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	

!IF  "$(CFG)" == "Ray - Win32 Release"


"$(INTDIR)\pvmstream.obj" : $(SOURCE) $(DEP_CPP_PVMST) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Ray - Win32 Debug"


"$(INTDIR)\pvmstream.obj" : $(SOURCE) $(DEP_CPP_PVMST) "$(INTDIR)"

"$(INTDIR)\pvmstream.sbr" : $(SOURCE) $(DEP_CPP_PVMST) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
