rem Programming fuses 
set AVRDUDE=avrdude.exe
set AVRCONF=avrdude.conf
%AVRDUDE% -C%AVRCONF% -v -v -v -v -patmega328p -cusbasp -Pusb -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xd8:m -Ulfuse:w:0xff:m
rem Programming complete flash 
%AVRDUDE% -C%AVRCONF% -v -v -v -v -patmega328p -cusbasp -Pusb -Uflash:w:FLASH_B2_FW12.HEX:i -Ulock:w:0x0F:m 