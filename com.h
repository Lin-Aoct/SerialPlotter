#ifndef COM_H
#define COM_H

#define COM_DEFAULT_DATA_BITS  QSerialPort::Data8
#define COM_DEFAULT_PARITY     QSerialPort::NoParity
#define COM_DEFAULT_STOP_BITS  QSerialPort::OneStop


typedef enum
{
    UTF_8,
    HEX
}Serial_Decode_Type;


#endif // COM_H
