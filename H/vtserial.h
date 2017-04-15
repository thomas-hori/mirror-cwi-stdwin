/* Definitions for serial interface */

/* Public interface to the serial line drivers (which have a
   machine-dependent implementation but a common interface) */

bool openserial _ARGS((void));
bool closeserial _ARGS((void));
bool sendserial _ARGS((char *buf, int len)); /* Blocking write */
int receiveserial _ARGS((char *buf, int len)); /* Non-blocking read */
bool breakserial _ARGS((void));
bool speedserial _ARGS((int baudrate));

/* Pseudo event reported when input from serial line available */

#define WE_SERIAL_AVAIL 42
