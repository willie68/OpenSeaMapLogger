rem Programming fuses 
C:\D-Platte\sprachen\Arduino\hardware\tools\avr\bin\avrdude -CC:\D-Platte\sprachen\Arduino\hardware\tools\avr\etc\avrdude.conf -v -v -v -v -patmega328p -cusbasp -Pusb -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xd8:m -Ulfuse:w:0xff:m
pause