@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by RAY.HPJ. >"help\Ray.hm"
echo. >>"help\Ray.hm"
echo // Commands (ID_* and IDM_*) >>"help\Ray.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 include\resource.h >>"help\Ray.hm"
echo. >>"help\Ray.hm"
echo // Prompts (IDP_*) >>"help\Ray.hm"
makehm IDP_,HIDP_,0x30000 include\resource.h >>"help\Ray.hm"
echo. >>"help\Ray.hm"
echo // Resources (IDR_*) >>"help\Ray.hm"
makehm IDR_,HIDR_,0x20000 include\resource.h >>"help\Ray.hm"
echo. >>"help\Ray.hm"
echo // Dialogs (IDD_*) >>"help\Ray.hm"
makehm IDD_,HIDD_,0x20000 include\resource.h >>"help\Ray.hm"
echo. >>"help\Ray.hm"
echo // Frame Controls (IDW_*) >>"help\Ray.hm"
makehm IDW_,HIDW_,0x50000 include\resource.h >>"help\Ray.hm"
REM -- Make help for Project RAY


echo Building Win32 Help files
start /wait hcrtf -x "help\Ray.hpj"
echo.
if exist Debug\nul copy "help\Ray.hlp" Debug
if exist Debug\nul copy "help\Ray.cnt" Debug
if exist Release\nul copy "help\Ray.hlp" Release
if exist Release\nul copy "help\Ray.cnt" Release
echo.


