@echo off
setlocal

::::
:::::::: Run the .bat as Administrator in the folder containing your .sys, .inf, and inf2cat.exe.
:::::::: Adjust kit_root as needed
::::

REM --- CONFIGURATION ------------------------------
set INF_NAME=SafeRing0Driver.inf
set SYS_NAME=SafeRing0Driver.sys
set CAT_NAME=SafeRing0Driver.cat
set DRIVER_NAME=SafeRing0Driver
set SIGNED_DIR=signed
::
set KIT_ROOT=C:\kits\10
::

REM --- CLEAN OUTPUT -------------------------------
if exist %SIGNED_DIR% rmdir /s /q %SIGNED_DIR%
mkdir %SIGNED_DIR%

REM --- GENERATE .CAT FILE -------------------------
"%KIT_ROOT%\bin\x64\inf2cat.exe" /driver:. /os:10_X64 /verbose
if errorlevel 1 (
    echo ERROR: inf2cat failed.
    pause
    exit /b 1
)

REM --- SIGN DRIVER --------------------------------
REM NOTE: Update this with your test or EV cert thumbprint
set SIGN_CERT=YOUR_CERT_THUMBPRINT_HERE

"%KIT_ROOT%\bin\x64\signtool.exe" sign ^
  /fd SHA256 /a /sha1 %SIGN_CERT% /tr http://timestamp.digicert.com /td sha256 ^
  /v %SYS_NAME% %CAT_NAME%

if errorlevel 1 (
    echo ERROR: signtool failed.
    pause
    exit /b 1
)

REM --- COPY FILES TO SIGNED DIR -------------------
copy %SYS_NAME% %SIGNED_DIR%\
copy %CAT_NAME% %SIGNED_DIR%\
copy %INF_NAME% %SIGNED_DIR%\

echo.
echo DONE. Signed files are in the %SIGNED_DIR% folder.
pause
