@echo off
echo %CD%
echo building the bootloader
call build.cmd
rem echo programming the fuses
rem cd \E-Platte\dropbox\Elektronik\Arduino\projekte\MCSDepthLogger\Software\OpenSeaMapLogger\bootloader\
rem start /wait fuses.cmd
echo burning the bootloader
cd \E-Platte\dropbox\Elektronik\Arduino\projekte\MCSDepthLogger\Software\OpenSeaMapLogger\bootloader\
call burn.cmd
pause