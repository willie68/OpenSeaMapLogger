/**
* here all NMEA messages are defined
*/
#define VERSIONNUMBER 12
#define VERSION PSTR("V 0.1.12")
#define START_MESSAGE PSTR("POSMST,Start NMEA Logger,V 0.1.12")
#define STOP_MESSAGE PSTR("POSMSO,Stop NMEA Logger")
#define REASON_TIME_MESSAGE PSTR("POSMSO,Reason: times up")
#define REASON_NODATA_MESSAGE PSTR("POSMSO,Reason: no data file")
#define REASON_VCC_MESSAGE PSTR("POSMSO,Reason: supply low")

// voltage message, value is voltage in mV
#define VCC_MESSAGE PSTR("POSMVCC,%i")
// gyroscope x,y,z axis
#define GYRO_MESSAGE PSTR("POSMGYR,%i,%i,%i")
// accelerator, x,y,z axis
#define ACC_MESSAGE PSTR("POSMACC,%i,%i,%i")
// timestamp in format hh:mm:ss.SSS
#define TIMESTAMP PSTR("%02d:%02d:%02d.%03u;")
// seatalk start, the datagram follows in hex
#define SEATALK_NMEA_MESSAGE PSTR("POSMSK,")
#define MAX_NMEA_BUFFER 80
#define DATA_FILENAME PSTR("data0000.dat")
#define CONFIG_FILENAME PSTR("config.dat")
#define CNF_FILENAME PSTR("oseamlog.cnf")

#define CHANNEL_A_IDENTIFIER 'A'
#define CHANNEL_B_IDENTIFIER 'B'
#define CHANNEL_I_IDENTIFIER 'I'

