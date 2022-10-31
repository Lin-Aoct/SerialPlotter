#include "rtt_stlink.h"

stlink_t *STLinkHdl = NULL;

/**
 * @description:
 * @param {uint8_t} *des
 * @param {uint32_t} addr
 * @param {uint32_t} len
 * @return {*}
 */
int read_mem(uint8_t *des, uint32_t addr, uint32_t len)
{
    uint32_t offsetAddr = 0, offsetLen, readLen;

    // address and read len need to align to 4
    readLen = len;
    offsetAddr = addr % 4;
    if (offsetAddr > 0)
    {
        addr -= offsetAddr;
        readLen += offsetAddr;
    }
    offsetLen = readLen % 4;
    if (offsetLen > 0)
        readLen += (4 - offsetLen);

    stlink_read_mem32(STLinkHdl, addr, readLen);

    // read data we actually need
    for (uint32_t i = 0; i < len; i++)
        des[i] = (uint8_t)STLinkHdl->q_buf[i + offsetAddr];

    return 0;
}

int write_mem(uint8_t *buf, uint32_t addr, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        STLinkHdl->q_buf[i] = buf[i];

    stlink_write_mem8(STLinkHdl, addr, len);
    return 0;
}

int get_channel_data(uint8_t *buf, rtt_channel *rttChannel, uint32_t rttChannelAddr)
{
    uint32_t len;

    if (rttChannel->WrOff > rttChannel->RdOff)  // 若 写地址偏移 大于 读地址偏移
    {
        len = rttChannel->WrOff - rttChannel->RdOff;
        read_mem(buf, rttChannel->pBuffer + rttChannel->RdOff, len);
        rttChannel->RdOff += len;
        write_mem((uint8_t *)&(rttChannel->RdOff), rttChannelAddr + 4 * 4, 4);
        return 1;
    }
    else if (rttChannel->WrOff < rttChannel->RdOff)  // 若 读地址偏移 大于 写地址偏移
    {
        len = rttChannel->SizeOfBuffer - rttChannel->RdOff;
        read_mem(buf, rttChannel->pBuffer + rttChannel->RdOff, len);
        read_mem(buf + len, rttChannel->pBuffer, rttChannel->WrOff);
        rttChannel->RdOff = rttChannel->WrOff;
        write_mem((uint8_t *)&(rttChannel->RdOff), rttChannelAddr + 4 * 4, 4);
        return 1;
    }
    else
    {
        return 0;
    }
}

RTT_Error_TypeDef rtt_stlink_init(rtt_cb *pRTTControlBlock)
{
    if (STLinkHdl == NULL)
    {
        STLinkHdl = stlink_open_first();

        if (STLinkHdl == NULL)
        {
            return STLINK_OPEN_FAILED;
        }

        STLinkHdl->verbose = 1;

        if (stlink_current_mode(STLinkHdl) == STLINK_DEV_DFU_MODE)
            stlink_exit_dfu_mode(STLinkHdl);

        if (stlink_current_mode(STLinkHdl) != STLINK_DEV_DEBUG_MODE)
            stlink_enter_swd_mode(STLinkHdl);

        // read the whole RAM
        uint8_t *buf = (uint8_t *)malloc(STLinkHdl->sram_size);
        uint32_t readCnt = STLinkHdl->sram_size / 0x400;
        printf("target have %u k ram\n\r", readCnt);
        for (uint32_t i = 0; i < readCnt; i++)
        {
            stlink_read_mem32(STLinkHdl, 0x20000000 + i * 0x400, 0x400);
            for (uint32_t k = 0; k < 0x400; k++)
                (buf + i * 0x400)[k] = (uint8_t)(STLinkHdl->q_buf[k]);
        }

        pRTTControlBlock = (rtt_cb *)malloc(sizeof(rtt_cb));
        pRTTControlBlock->cb_addr = 0;

        // 查找 SEGGER RTT Control Block 地址
        uint32_t offset;
        for (offset = 0; offset < STLinkHdl->sram_size - 16; offset++)
        {
            if (strncmp((char *)&buf[offset], "SEGGER RTT", 16) == 0)
            {
                pRTTControlBlock->cb_addr = 0x20000000 + offset;
                printf("addr = 0x%x\n\r", pRTTControlBlock->cb_addr);
                break;
            }
        }

        // 未找到 SEGGER RTT Control Block 地址
        if (pRTTControlBlock->cb_addr == 0)
        {
            free(buf);
            return SEGGER_CONTROL_BLOCK_NOT_FOUND;
        }

        // get SEGGER_RTT_CB content
        memcpy(pRTTControlBlock->acID, ((rtt_cb *)(buf + offset))->acID, 16);
        pRTTControlBlock->MaxNumUpBuffers = ((rtt_cb *)(buf + offset))->MaxNumUpBuffers;
        pRTTControlBlock->MaxNumDownBuffers = ((rtt_cb *)(buf + offset))->MaxNumDownBuffers;
        pRTTControlBlock->cb_size = 24 + (pRTTControlBlock->MaxNumUpBuffers + pRTTControlBlock->MaxNumDownBuffers) * sizeof(rtt_cb);
        pRTTControlBlock->aUp = (rtt_channel *)malloc(pRTTControlBlock->MaxNumUpBuffers * sizeof(rtt_channel));
        pRTTControlBlock->aDown = (rtt_channel *)malloc(pRTTControlBlock->MaxNumDownBuffers * sizeof(rtt_channel));
        memcpy(pRTTControlBlock->aUp, buf + offset + 24, pRTTControlBlock->MaxNumUpBuffers * sizeof(rtt_channel));
        memcpy(pRTTControlBlock->aDown, buf + offset + 24 + pRTTControlBlock->MaxNumUpBuffers * sizeof(rtt_channel),
                pRTTControlBlock->MaxNumDownBuffers * sizeof(rtt_channel));

        free(buf);
        stlink_run(STLinkHdl, RUN_NORMAL);
    }
}

void rtt_stlink_close(rtt_cb *pRTTControlBlock)
{
    stlink_exit_debug_mode(STLinkHdl);
    stlink_close(STLinkHdl);
    STLinkHdl = NULL;
    free(pRTTControlBlock->aUp);
    free(pRTTControlBlock->aDown);
    free(pRTTControlBlock);
}

static stlink_t *stlink_open_first(void)
{
    stlink_t *STLinkHdl = NULL;
    STLinkHdl = stlink_v1_open(0, 1);
    if (STLinkHdl == NULL)
        STLinkHdl = stlink_open_usb(0, CONNECT_NORMAL, NULL, 0);
        // STLinkHdl = stlink_open_usb(0, 1, NULL);

    return STLinkHdl;
}

uint8_t is_stlink_opened(void)
{
    return (STLinkHdl == NULL)? STLINK_NOT_OPEN : STLINK_OPENED;
}
