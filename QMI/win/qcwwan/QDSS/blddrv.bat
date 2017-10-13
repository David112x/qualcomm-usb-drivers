echo off

REM need to define QCUSB_DDK_DIR,    example: set QCUSB_DDK_DIR=C:\NTDDK
REM need to define QCUSB_SOURCE_DIR, example: set QCUSB_SOURCE_DIR=C:\usb\src
REM need to define QCUSB_TARGET_DIR, example: set QCUSB_TARGET_DIR=C:\usb\cs\target

echo.
echo  -------------------------------------------------------
echo  ^|       Qualcomm USB Host Driver Building Process      ^|
echo  ^| Copyright (c) 2012 QUALCOMM Inc. All Rights Reserved ^|
echo  --------------------------------------------------------
echo.

if "%1"=="/?" goto usage
if "%1"=="-?" goto usage
if "%1"=="\?" goto usage
if "%1"==""   goto usage

if "%1"=="/wdk7000" goto build_on_w7
goto usage

REM -------- Build with WDK 7000 ---------
:build_on_w7
echo Compliling Windows 7 x86 checked build with WDK
REM start /I "Win7 x86 Debug Build " /MAX /WAIT CMD.EXE /C qcpl.bat -7 x86v -d

echo Compliling Windows 7 x86 free build with WDK
start /I "Win7 x86 Free Build " /MAX /WAIT CMD.EXE /C qcpl.bat -7 x86v

echo Compliling Windows 7 AMD64 checked/debug build with WDK
REM start /I "AMD64 Debug Build " /MAX /WAIT CMD.EXE /C qcpl.bat -7 amd64v -d

echo Compliling Windows 7 AMD64 free/non-debug build with WDK
start /I "AMD64 free Build " /MAX /WAIT CMD.EXE /C qcpl.bat -7 amd64v

goto EndOfBuild

:EndOfBuild
echo     ............. End of Build Process .............
echo.
echo.
goto EndAll

:usage
echo.
echo usage: blddrv ^</wdk7000^>
echo.
echo   Example:  blddrv /wdk7000 - build images with WDK 7000
echo   Copyright (c) 2012 by Qualcomm Incorporated. All rights reserved.
echo.
echo.

:EndAll
