logger-oseam-0183
=================

Hardware-logger with 2 channels for NMEA-0183 devices, one channel switchable to Seatalk.

Manual http://wiki.openseamap.org/wiki/OpenSeaMap-dev:HW-logger/OSeaM-Manual

HowTo http://wiki.openseamap.org/wiki/OpenSeaMap-dev:HW-logger/OSeaM

Specification http://wiki.openseamap.org/wiki/OpenSeaMap-dev:HW-logger/specification#NMEA-0183_HW-Logger

Developerpage with all actual informations http://www.wk-music.de/ArduinoWiki/doku.php?id=arduino:oseam

OpenSeaMapLogger
================
Einleitung

Der Datenlogger arbeitet nach einem einfachen Prinzip. Nach Anlegen der Betriebsspannung und einer Initialisierungszeit werden automatisch die Daten, die an den Anschlüssen NMEA A bzw. NMEA B anliegen auf die Karte geschrieben. Dabei erfolgt keinerlei Filterung. Dazu werden dann automatisch die Lagedaten des Loggers mit protokoliert.
Datenlogger für GPS und Echolot Daten.
- Eingebauter Lagesensor
- Daten auf normaler SD Karte im NMEA 0183 Format
- Einfachste Bedienung. 
- LED für Betrieb, Datenempfang und SD Zugriff
- zwei NMEA0183 Eingänge (4800 Baud, 8N1 Protokoll)
- ein SeaTalk 1 Eingang (Experimentell)


How to build on Linux
=====================

this worked on Ubuntu 12.04

install arduino IDE
> sudo apt-get install arduino

then get the source 
(create a fork before on github, if you want to contribute code)
> cd src           # or where you store your sources
> git clone https://github.com/OpenSeaMap/logger-oseam-0183.git
> MyOslGit=`pwd`/logger-oseam-0183

then create this dirty link to our hardware, so arduino ide knows it
see also $MyOslGit/content.txt
> sudo ln -s $MyOslGit/SketchBook/hardware/OSMLogger/avr /usr/share/arduino/hardware/

now start the IDE
> arduino 
menu file/preferences: set $MyOslGit as your SketchBook directory 
icon open: OpenSeaMap
menu tools: board  - select OpenSeaMap data logger board w/ATmega328
menu sketch: Ctrl-R  - compile
> ls -trl /tmp   # here goes the hex image in a subfolder...