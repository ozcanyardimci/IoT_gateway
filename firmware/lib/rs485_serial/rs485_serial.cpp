#include "rs485_serial.h"
#include "pin_map.h"

void Rs485Serial::begin(unsigned long baud) {
    serial_.begin(baud, SERIAL_8N1, PIN_RS485_RXD, PIN_RS485_TXD);
}

HardwareSerial &Rs485Serial::port() {
    return serial_;
}
