rem Programming fuses 
rem C:\D-Platte\sprachen\Arduino\hardware\tools\avr\bin\avrdude -CC:\D-Platte\sprachen\Arduino\hardware\tools\avr\etc\avrdude.conf -v -v -v -v -patmega328p -cusbasp -Pusb -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xd8:m -Ulfuse:w:0xff:m
rem Programming bootloader 
rem C:\D-Platte\sprachen\Arduino\hardware\tools\avr\bin\avrdude -CC:\D-Platte\sprachen\Arduino\hardware\tools\avr\etc\avrdude.conf -v -v -v -v -patmega328p -cusbasp -Pusb -Uflash:w:C:\E-Platte\dropbox\Elektronik\Arduino\projekte\MCSDepthLogger\Software\OpenSeaMapLogger\bootloader\2boot\build\2boots-osm-atmega328p-16000000L-PB2.hex:i -Ulock:w:0x0F:m 
C:\D-Platte\sprachen\Arduino\hardware\tools\avr\bin\avrdude -CC:\D-Platte\sprachen\Arduino\hardware\tools\avr\etc\avrdude.conf -v -v -v -v -patmega328p -cusbasp -Pusb -Uflash:w:C:\E-Platte\dropbox\Elektronik\Arduino\projekte\MCSDepthLogger\Software\OpenSeaMapLogger\bootloader\avr_boot-master\avr_boot.hex:i -Ulock:w:0x0F:m 

rem C:\D-Platte\sprachen\Arduino\hardware\tools\avr\bin\avrdude -CC:\D-Platte\sprachen\Arduino\hardware\tools\avr\etc\avrdude.conf -v -v -v -v -patmega328p -cusbasp -Pusb -Uflash:w:C:\E-Platte\dropbox\Elektronik\Arduino\projekte\MCSDepthLogger\Software\OpenSeaMapLogger\bootloader\pfboot\pfboot.hex:i -Ulock:w:0x0F:m 
