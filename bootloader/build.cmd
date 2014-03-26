@echo off
c:
rem cd \E-Platte\dropbox\Elektronik\Arduino\projekte\MCSDepthLogger\Software\OpenSeaMapLogger\bootloader\2boots\
cd avr_boot-master\
rem cd pfboot\
SET PATH=C:\D-Platte\sprachen\Atmel\Atmel Studio 6.1\shellUtils;C:\D-Platte\sprachen\Atmel\Atmel Toolchain\AVR8 GCC\Native\3.4.2.1002\avr8-gnu-toolchain\bin
make clean
make
cd ..