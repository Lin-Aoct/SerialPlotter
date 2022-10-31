#ifndef RTT_STLINK_H
#define RTT_STLINK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <stlink.h>

#define STLINK_NOT_OPEN 0
#define STLINK_OPENED   1

typedef struct
{
    uint32_t sName;        // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
    uint32_t pBuffer;      // Pointer to start of buffer
    uint32_t SizeOfBuffer; // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
    uint32_t WrOff;        // Position of next item to be written by either target.
    uint32_t RdOff;        // Position of next item to be read by host. Must be volatile since it may be modified by host.
    uint32_t Flags;        // Contains configuration flags
} rtt_channel;

typedef struct
{
    int8_t acID[16];           // Initialized to "SEGGER RTT"
    int32_t MaxNumUpBuffers;   // Initialized to SEGGER_RTT_MAX_NUM_UP_BUFFERS (type. 2)
    int32_t MaxNumDownBuffers; // Initialized to SEGGER_RTT_MAX_NUM_DOWN_BUFFERS (type. 2)
    int32_t cb_size;
    int32_t cb_addr;
    rtt_channel *aUp;   // Up buffers, transferring information up from target via debug probe to host
    rtt_channel *aDown; // Down buffers, transferring information down from host via debug probe to target
} rtt_cb;               // RTT Control Block

typedef enum
{
    STLINK_OPEN_FAILED = 0u,
    SEGGER_CONTROL_BLOCK_NOT_FOUND,
} RTT_Error_TypeDef;


int read_mem(uint8_t *des, uint32_t addr, uint32_t len);
int write_mem(uint8_t *buf, uint32_t addr, uint32_t len);
int get_channel_data(uint8_t *buf, rtt_channel *rttChannel, uint32_t rttChannelAddr);
RTT_Error_TypeDef rtt_stlink_init(rtt_cb *pRTTControlBlock);
void rtt_stlink_close(rtt_cb *pRTTControlBlock);
static stlink_t *stlink_open_first(void);
uint8_t is_stlink_opened(void);

#endif
