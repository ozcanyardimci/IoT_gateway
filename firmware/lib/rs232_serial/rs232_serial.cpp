#include "rs232_serial.h"
#include "pin_map.h"

void Rs232Serial::begin(unsigned long baud) {
    serial_.begin(baud, SERIAL_8N1, PIN_RS232_RXD, PIN_RS232_TXD);
}

HardwareSerial &Rs232Serial::port() {
    return serial_;
}
