@echo off

REM ============== About Vista Test Certificate =================
REM Ignoe the following steps if the driver is built with a DDK other
REM than the Vista WDK.
REM
REM In order to test sign the driver for Windows Vista,
REM follow the steps below:
REM
REM 1) On the driver build PC (Vista), open any WDK build environment
REM    and make a directory, for example
REM    C:>md MyCert
REM
REM 2) In the WDK build environment, create a test certificate in the
REM    certificate store trustedpublisher. For example,
REM    C:\MyCert>makecert -r -pe -ss trustedpublisher -n CN=USBHostDriver(Test002) qcusbtest.cer
REM    This generates a certificate file qcusbtest.cer in C:\MyCert
REM 
REM 3) Add the certificate (qcusbtest.cer) to the root store. For example,
REM    C:\MyCert>Certmgr.exe -add qcusbtest.cer -s -r localMachine root
REM
REM 4) If a user uses a different certificate name other than USBHostDriver(Test002),
REM    make sure the QCUSB_CERT_NAME in this script is set to the new name.
REM
REM 5) Make sure the build PC is connected to the internet.
REM
REM 6) Build the driver and the driver binaries will be test signed. For example,
REM    C:\Driver>blddrv /wdk6k
REM
REM If the driver is to be installed on another Vista PC, install the certificate first
REM on that PC. For example,
REM    C:\DriverCert>Certmgr.exe -add qcusbtest.cer -s -r localMachine root
REM    C:\DriverCert>Certmgr.exe -add qcusbtest.cer -s -r localMachine trustedpublisher
REM 
REM ==============================================================

set QDBUSB_BLD_VERSION=1.0.0.6

set QCUSB_CERT_NAME=USBHostDriver(Test003)

if "%QCUSB_DDK_DRIVE%"=="" set QCUSB_DDK_DRIVE=C:

if "%QCUSB_SOURCE_DIR%"=="" set QCUSB_SOURCE_DIR=%CD%
if not exist %QCUSB_SOURCE_DIR% goto error_path_src

if "%QCUSB_TARGET_DIR%"==""     set QCUSB_TARGET_DIR=%QCUSB_SOURCE_DIR%
set QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%\target
set QCUSB_VISTA_BUILD=no
set QCUSB_CERT_FILE=%QCUSB_SOURCE_DIR%\private\qcusbtest.pfx
set QCUSB_TEST_CERT=%QCUSB_SOURCE_DIR%\private\stuff
if not exist %QCUSB_TARGET_DIR%\TestCertificate md %QCUSB_TARGET_DIR%\TestCertificate
if not exist %QCUSB_TARGET_DIR%\TestCertificate\qcusbtest.cer goto copy_test_cert
goto build_start

:copy_test_cert
copy %QCUSB_TEST_CERT%\qcusbtest.cer %QCUSB_TARGET_DIR%\TestCertificate
copy %QCUSB_TEST_CERT%\README.txt    %QCUSB_TARGET_DIR%\TestCertificate
if exist  %QCUSB_SOURCE_DIR%\certmgr.exe copy %QCUSB_SOURCE_DIR%\certmgr.exe %QCUSB_TARGET_DIR%\TestCertificate
goto build_start

:error_path_ddk
echo ERROR: Invalid DDK path: %QCUSB_DDK_DIR%
goto end_all

:error_path_src
echo ERROR: Invalid source path: %QCUSB_SOURCE_DIR%
goto end_all

:build_start
echo QCUSB_DDK_DIR=%QCUSB_DDK_DIR%
echo QCUSB_SOURCE_DIR=%QCUSB_SOURCE_DIR%
echo QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%

if "%1"=="/?" goto usage
if "%1"=="-?" goto usage
if "%1"=="\?" goto usage
if "%1"=="" goto usage

if "%1"=="-7" goto build_from_w7
goto usage

:build_from_w7
if "%QCUSB_DDK_DIR%"==""    set QCUSB_DDK_DIR=%QCUSB_DDK_DRIVE%\WINDDK\7600.16385.1
if not exist %QCUSB_DDK_DIR%    goto error_path_ddk

if "%2"=="x86v" goto build_w7_x86_driver_from_w7
if "%2"=="amd64v" goto build_w7_amd64_driver_from_w7
goto usage

:build_w7_x86_driver_from_w7
echo QCUSB_DDK_DIR=%QCUSB_DDK_DIR%
echo QCUSB_SOURCE_DIR=%QCUSB_SOURCE_DIR%
echo QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%
set QCUSB_VISTA_BUILD=yes
if "%3"=="-d" goto build_w7_x86_driver_from_w7_dbg
if "%3"==""   goto build_w7_x86_driver_from_w7_free
goto usage

:build_w7_amd64_driver_from_w7
set QCUSB_VISTA_BUILD=yes
if "%3"=="-d" goto build_w7_amd64_driver_from_w7_dbg
if "%3"==""   goto build_w7_amd64_driver_from_w7_free
goto usage

REM *********************************************
REM ***************** WDK 7000 ******************
REM *********************************************
:build_w7_x86_driver_from_w7_dbg
echo ================ build_w7_x86_from_w7_dbg ==================
call %QCUSB_DDK_DIR%\bin\setenv.bat %QCUSB_DDK_DIR% chk x86 WIN7 no_oacr
cd /D %QCUSB_SOURCE_DIR%
copy SOURCES.CHK SOURCES
set QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%\Win32\x86\checked
set QCUSB_SIGN_TOOL=%QCUSB_DDK_DIR%\bin\x86
set QCUSB_REDIST_DLL=%QCUSB_DDK_DIR%\redist\wdf\x86
build -e -w -c
cd objchk_win7_x86\i386
goto post_build

:build_w7_x86_driver_from_w7_free
echo ================ build_w7_x86_from_w7_fre ==================
call %QCUSB_DDK_DIR%\bin\setenv.bat %QCUSB_DDK_DIR% fre x86 WIN7 no_oacr
cd /D %QCUSB_SOURCE_DIR%
copy SOURCES.FRE SOURCES
set QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%\Win32\x86\free
set QCUSB_SIGN_TOOL=%QCUSB_DDK_DIR%\bin\x86
set QCUSB_REDIST_DLL=%QCUSB_DDK_DIR%\redist\wdf\x86
build -e -w -c
cd objfre_win7_x86\i386
goto post_build

:build_w7_amd64_driver_from_w7_dbg
echo ================ build_w7_amd64_from_w7_dbg ==================
call %QCUSB_DDK_DIR%\bin\setenv.bat %QCUSB_DDK_DIR% chk x64 WIN7 no_oacr
cd /D %QCUSB_SOURCE_DIR%
copy SOURCES.CHK SOURCES
set QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%\Win64\AMD64\checked
set QCUSB_SIGN_TOOL=%QCUSB_DDK_DIR%\bin\x86
set QCUSB_REDIST_DLL=%QCUSB_DDK_DIR%\redist\wdf\amd64
build -e -w -c
cd objchk_win7_amd64\amd64
goto post_build

:build_w7_amd64_driver_from_w7_free
echo ================ build_w7_amd64_from_w7_fre ==================
call %QCUSB_DDK_DIR%\bin\setenv.bat %QCUSB_DDK_DIR% fre x64 WIN7 no_oacr
cd /D %QCUSB_SOURCE_DIR%
copy SOURCES.FRE SOURCES
set QCUSB_TARGET_DIR=%QCUSB_TARGET_DIR%\Win64\AMD64\free
set QCUSB_SIGN_TOOL=%QCUSB_DDK_DIR%\bin\x86
set QCUSB_REDIST_DLL=%QCUSB_DDK_DIR%\redist\wdf\amd64
build -e -w -c
cd objfre_win7_amd64\amd64
goto post_build

REM *********************************************
REM ***************** Post Build ****************
REM *********************************************
:post_build
if not exist %QCUSB_TARGET_DIR% md %QCUSB_TARGET_DIR%
copy qdbusb*.sys %QCUSB_TARGET_DIR%
copy qdbusb*.pdb %QCUSB_TARGET_DIR%
copy %QCUSB_REDIST_DLL%\WdfCoInstaller01009.dll %QCUSB_TARGET_DIR%

dir qdbusb*.sys

cd /D %QCUSB_SOURCE_DIR%
del *.sbr
copy README.txt    %QCUSB_TARGET_DIR%
copy qdbusb.inf    %QCUSB_TARGET_DIR%

if "%1"=="-l" goto gen_cat_file
if "%1"=="-7" goto gen_cat_file
goto file_cleanup

REM create .cat file
:gen_cat_file
copy qdbusb.cdf %QCUSB_TARGET_DIR%
cd /D %QCUSB_TARGET_DIR%
stampinf -f qdbusb.inf -d * -v %QDBUSB_BLD_VERSION%
%QCUSB_SIGN_TOOL%\makecat /v qdbusb.cdf
del qdbusb.cdf


REM Test-sign the driver if built within Qualcomm NA domain
REM if /i "%USERDNSDOMAIN%"=="na.qualcomm.com" goto test_sign_catalog
goto test_sign_catalog
goto file_cleanup

:test_sign_catalog
if exist %QCUSB_CERT_FILE% goto sign_with_file
%QCUSB_SIGN_TOOL%\signtool.exe sign /v /s trustedpublisher /n %QCUSB_CERT_NAME% /t http://timestamp.verisign.com/scripts/timstamp.dll qdbusb.cat
goto file_cleanup

:sign_with_file
echo Test sign with Personal Information Exchange (PFX) file
%QCUSB_SIGN_TOOL%\signtool.exe sign /v /f %QCUSB_CERT_FILE% -p testcert /t http://timestamp.verisign.com/scripts/timstamp.dll qdbusb.cat

:file_cleanup
cd /D %QCUSB_SOURCE_DIR%
REM === remove intermidiate files ===
del *.log

if exist %QCUSB_SOURCE_DIR%\objchk_win7_amd64 rd /S/Q %QCUSB_SOURCE_DIR%\objchk_win7_amd64
if exist %QCUSB_SOURCE_DIR%\objchk_win7_ia64 rd /S/Q %QCUSB_SOURCE_DIR%\objchk_win7_ia64
if exist %QCUSB_SOURCE_DIR%\objchk_win7_x86 rd /S/Q %QCUSB_SOURCE_DIR%\objchk_win7_x86
if exist %QCUSB_SOURCE_DIR%\objfre_win7_amd64 rd /S/Q %QCUSB_SOURCE_DIR%\objfre_win7_amd64
if exist %QCUSB_SOURCE_DIR%\objfre_win7_ia64 rd /S/Q %QCUSB_SOURCE_DIR%\objfre_win7_ia64
if exist %QCUSB_SOURCE_DIR%\objfre_win7_x86 rd /S/Q %QCUSB_SOURCE_DIR%\objfre_win7_x86

goto end

:usage

echo.
echo usage: cpl ^<-x^|-k^|-l^> [xp^|2k^|lh] [-d]
echo.
echo   Example:  cpl -x xp -d   build checked XP driver under XP DDK
echo   Example:  cpl -k 2k      build free Win2K driver under 2K DDK
echo   Example:  cpl -l lh      build free Vista driver under WDK
echo   Copyright (c) 2007 by Qualcomm Incorporated. All rights reserved.
echo.
echo.

:end
if exist %QCUSB_SOURCE_DIR%\SOURCES del %QCUSB_SOURCE_DIR%\SOURCES

:end_all
pause
