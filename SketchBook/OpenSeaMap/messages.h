/**
* here all NMEA messages are defined
*/
#define START_MESSAGE PSTR("POSMST,Start NMEA Logger,V 0.1.3")
#define STOP_MESSAGE PSTR("POSMSO,Stop NMEA Logger")
#define VCC_MESSAGE PSTR("POSMVCC,%i")
#define GYRO_MESSAGE PSTR("POSMGYR,%i,%i,%i")
#define ACC_MESSAGE PSTR("POSMACC,%i,%i,%i")
#define TIMESTAMP PSTR("%02d:%02d:%02d.%03u;")
#define SEATALK_NMEA_MESSAGE PSTR("POSMSK,")
#define MAX_NMEA_BUFFER 80


