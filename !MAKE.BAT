rem call c:\opwat20\setvars.bat
rem call c:\_wat_c_\setvars.bat
SET INCLUDEBEFORE=%INCLUDE%
SET INCLUDE=%INCLUDE%;SRC\STL;SRC\IMGUI;
SET PMODEWCOMPILE=0
_build\text.exe
@echo off
@echo Starting.. (needs RamDisk at R:)

rem pmodew has currently a problem with the exe size or the raw program size (only WatcomC++11) but with hdpmi32i of SBEMU it runs also well with DOS4GW

if %PMODEWCOMPILE%==1 goto label2
  @echo system dos4g > R:\LINK.LNK
:label2

if %PMODEWCOMPILE%==0 goto label3
  @echo system pmodew > R:\LINK.LNK
:label3

@echo build errors > R:\LOGS.LOG
@echo -oR:\main.exe > R:\LINK2.LNK
_BUILD\FLTMECHK.exe !MAK R:\main.exe
if not ERRORLEVEL 255 call _BUILD\MAKEMK.BAT
SET DJGPPCOMPILE=0
call wmake.exe /f R:\makefile /e /s /k
if %DJGPPCOMPILE%==0 vcode.exe R:\logs.log
SET DJGPPCOMPILE=0
set INCLUDE=%INCLUDEBEFORE%
