#ifndef RTTJLINK_HPP
#define RTTJLINK_HPP

#include <QObject>
#include <QDebug>
#include <windows.h>
#include <QLibrary>

#define SEGGER_RTT_MAX_NUM_UP_BUFFERS       2
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS     2

class RTTJLink : public QObject
{
    Q_OBJECT
public:
    explicit RTTJLink(QObject *parent = nullptr);

private:
    typedef void(*_ptrOpen)(void);
    typedef int(*_ptrIsOpened)(void);



    typedef struct {
        const char* sName;              // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
        char*       pBuffer;            // Pointer to start of buffer
        unsigned    SizeOfBuffer;       // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
        volatile    unsigned WrOff;     // Position of next item to be written by either host (down-buffer) or target (up-buffer). Must be volatile since it may be modified by host (down-buffer)
        volatile    unsigned RdOff;     // Position of next item to be read by target (down-buffer) or host (up-buffer). Must be volatile since it may be modified by host (up-buffer)
        unsigned    Flags;              // Contains configuration flags
    } SEGGER_RTT_RING_BUFFER;

    // RTT control block which describes the number of buffers available
    // as well as the configuration for each buffer
    typedef struct {
        char                    acID[16];               // Initialized to "SEGGER RTT"
        int                     MaxNumUpBuffers;        // Initialized to SEGGER_RTT_MAX_NUM_UP_BUFFERS (type. 2)
        int                     MaxNumDownBuffers;      // Initialized to SEGGER_RTT_MAX_NUM_DOWN_BUFFERS (type. 2)
        SEGGER_RTT_RING_BUFFER  aUp[SEGGER_RTT_MAX_NUM_UP_BUFFERS];     // Up buffers, transferring information up from target via debug probe to host
        SEGGER_RTT_RING_BUFFER  aDown[SEGGER_RTT_MAX_NUM_DOWN_BUFFERS]; // Down buffers, transferring information down from host via debug probe to target
    } SEGGER_RTT_CB;


    HMODULE hJLinkDll;

signals:

public slots:
};

#endif // RTTJLINK_H
