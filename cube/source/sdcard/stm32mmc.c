/*------------------------------------------------------------------------*/
/* STM32F100: MMCv3/SDv1/SDv2 (SPI mode) control module                   */
/*------------------------------------------------------------------------*/
/*
/  Copyright (C) 2018, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include "../sys/stm32drv.h"
#include "../sys/log.h"
#include <string.h>

#if HWVER >= 2
#define SDCARD_PIN_CD GPIOA, GPIO_PIN_3
#endif // HWVER

extern SPI_HandleTypeDef hspi1;
extern RTC_HandleTypeDef hrtc;

static uint32_t clk_chg(uint32_t BaudRatePrescaler) {
    uint32_t _prv = hspi1.Instance->CR1 & SPI_CR1_BR_Msk;
    WRITE_REG(
        hspi1.Instance->CR1,
        (READ_REG(hspi1.Instance->CR1) & ~SPI_CR1_BR_Msk) | (BaudRatePrescaler & SPI_CR1_BR_Msk));
    return _prv;
}
static uint32_t clk_slow() {
    return clk_chg(SPI_BAUDRATEPRESCALER_256);
}

/*
static void clk_fast() {
    clk_chg(SPI_BAUDRATEPRESCALER_16);
}
*/

#if HWVER < 2
static void cs_hi() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
}
static void cs_lo() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
}
#else
void sdcard_on() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
}
void sdcard_off() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
}

static void cs_hi() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}
static void cs_lo() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}
#endif // HWVER

#define TMR(wait) uint32_t _tmr = HAL_GetTick() + wait;
#define TMROK (_tmr > HAL_GetTick())

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/
#include "../ff/ff.h"
#include "../ff/diskio.h"

/* MMC/SD command */
#define CMD0 (0)		   /* GO_IDLE_STATE */
#define CMD1 (1)		   /* SEND_OP_COND (MMC) */
#define CMD8 (8)		   /* SEND_IF_COND */
#define CMD9 (9)		   /* SEND_CSD */
#define CMD10 (10)		   /* SEND_CID */
#define CMD12 (12)		   /* STOP_TRANSMISSION */
#define CMD13 (13)         /* SD_STATUS (SDC) */
#define CMD16 (16)		   /* SET_BLOCKLEN */
#define CMD17 (17)		   /* READ_SINGLE_BLOCK */
#define CMD18 (18)		   /* READ_MULTIPLE_BLOCK */
#define CMD23 (23)		   /* SET_BLOCK_COUNT (MMC) */
#define CMD24 (24)		   /* WRITE_BLOCK */
#define CMD25 (25)		   /* WRITE_MULTIPLE_BLOCK */
#define CMD32 (32)		   /* ERASE_ER_BLK_START */
#define CMD33 (33)		   /* ERASE_ER_BLK_END */
#define CMD38 (38)		   /* ERASE */
#define CMD41 (41)         /* SEND_OP_COND (SDC) */
#define CMD42 (42)         /* APP_CLR_CARD_DETECT */
#define CMD55 (55)		   /* APP_CMD */
#define CMD58 (58)		   /* READ_OCR */
#define CMD59 (59)		   /* CRC_ON_OFF */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC3 0x01  /* MMC ver 3 */
#define CT_MMC4 0x02  /* MMC ver 4+ */
#define CT_MMC 0x03	  /* MMC */
#define CT_SDC1 0x04  /* SD ver 1 */
#define CT_SDC2 0x08  /* SD ver 2+ */
#define CT_SDC 0x0C	  /* SD */
#define CT_BLOCK 0x10 /* Block addressing */

static volatile DSTATUS Stat = STA_NOINIT; /* Physical drive status */
static BYTE CardType;					   /* Card type flags */

DWORD get_fattime(void) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (
        (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) ||
        (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)) {
        return 0;
    }

    /* Pack date and time into a DWORD variable */
    return ((DWORD)((uint16_t)(sDate.Year) + 2000 - 1980) << 25) | ((DWORD)sDate.Month << 21) | ((DWORD)sDate.Date << 16) | ((DWORD)sTime.Hours << 11) | ((DWORD)sTime.Minutes << 5) | ((DWORD)sTime.Seconds >> 1);
}

/*----------------------------------------------------------------------*/
/*  SD tx-rx commands 														*/
/*----------------------------------------------------------------------*/
static uint8_t _encrc = 1;
uint8_t CRC7(const uint8_t *d, int sz);
uint16_t CRC16(const uint8_t *d, int sz);

static void dummy(uint8_t sz) {
    uint8_t tx[sz];
    memset(tx, 0xff, sizeof(tx));
    HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), 200);
}

static void recv(uint8_t *rx, uint16_t sz) {
    uint8_t tx[sz];
    memset(tx, 0xff, sizeof(tx));
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, sz, 500);
}

static uint8_t rcv() {
    uint8_t tx = 0xff, rx;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);
    return rx;
}

static int wait(uint32_t wt /* Timeout [ms] */) { /* return 1:Ready, 0:Timeout */
    TMR(wt);
    while (1) {
        if (rcv() != 0x00)
            return 1;
        if (!TMROK)
            break;
    }

    CONSOLE("wait(%u) fail", wt);
    return 0;
}

static uint8_t recvblk(uint8_t *rx, uint16_t sz) {
    TMR(2000);
    while (1) {
        uint8_t r = rcv();
        if (r == 0xFE)
            break;
        if ((r != 0xff) || !TMROK)
            return 0;
    }
    
    recv(rx, sz);

    if (_encrc) {
        uint8_t c[2];
        recv(c, sizeof(c));
        uint16_t crc = c[0];
        crc = (crc << 8) | c[1];
        return crc == CRC16(rx, sz);
    }
    else {
        dummy(2); /* Discard CRC */
        return 1;
    }
}

static int sendblk(const uint8_t *tx, /* Ponter to 512 byte data to be sent */ uint8_t token /* Token */) {
    if (!wait(500))
        return 0; /* Wait for card ready */

    // Send token
    HAL_SPI_Transmit(&hspi1, &token, 1, 10);
    if (tx == NULL)
        return 1;
    
    // data
    HAL_SPI_Transmit(&hspi1, tx, 512, 1000);

    //crc
    if (_encrc) {
        uint16_t crc = CRC16(tx, 512);
        uint8_t c[2] = {
            (uint8_t)(crc >> 8),
            (uint8_t)crc
        };
        HAL_SPI_Transmit(&hspi1, c, 2, 10);
    }
    else
        /* Dummy CRC */
        dummy(2);

    /* Receive data resp */
    if ((rcv() & 0x1F) != 0x05)
        return 0; /* Function fails if the data packet was not accepted */
    
    return 1;
}

static uint8_t _act = 0;
static void beg() {
    cs_lo();
    dummy(1);
    _act = 1;
}
static void end() {
    cs_hi();
    dummy(1);
    _act = 0;
}
static uint8_t start() {
    if (_act)
        return 1;
    
    beg();
    if (wait(500))
        return 1; // ok

    end(); // timeout
    return 0;
}

static uint8_t _cmd(uint8_t cmd, uint32_t arg) {
    uint8_t tx[6] = {
        cmd | 0x40,
        (uint8_t)(arg >> 24),
        (uint8_t)(arg >> 16),
        (uint8_t)(arg >> 8),
        (uint8_t)arg
    };
    tx[5] =
        _encrc || (cmd == CMD0) || (cmd == CMD8) ?
            (CRC7(tx, 5) << 1) | 0x01 :
            0x01;
    //tx[6] = 0xff; // discard first fill byte to avoid MISO pull-up problem.
    HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), 100);

    //rcv(); // discard first fill byte to avoid MISO pull-up problem.

    uint8_t r;
    /* Wait for response (10 bytes max) */
    for (int i = 0; i < 10; i++) {
        r = rcv();
        if (!(r & 0x80))
            break;
    }

    if (r == 0xFF)
        CONSOLE("[%u] no token received", cmd);
    else
    if (r & 0x08)
        CONSOLE("[%u] crc error", cmd);
    else
    if (r > 1)
        CONSOLE("[%u] token error 0x%x", cmd, r);
    
    if (r > 1)
        end();

    return r;
}

static uint8_t rcmd(uint8_t cmd, uint32_t arg) {
    if (!start())
        return 0xff;
    
    return _cmd(cmd, arg);
}

static uint8_t scmd(uint8_t cmd, uint32_t arg) {
    uint8_t r = rcmd(cmd, arg);
    end();
    return r;
}

static uint8_t acmd(uint8_t cmd, uint32_t arg) {
    if (!start())
        return 0xff;

    uint8_t r = _cmd(CMD55, 0);
    if (r > 1)
        return r;
    end();
    if (!start())
        return 0xff;
    
    r = _cmd(cmd, arg);
    end();

    return r;
}

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE drv /* Physical drive number (0) */
) {
    if (drv)
        return STA_NOINIT; /* Supports only drive 0 */

    if (Stat & STA_NODISK)
        return Stat; /* Is card existing in the soket? */

    uint32_t cperv = clk_slow();
#define fail(...)   do { clk_chg(cperv); end(); CardType = 0; CONSOLE(__VA_ARGS__); return Stat; } while (0)

    cs_hi();
    // must supply min of 74 clock cycles with CS high.
    dummy(10);
    beg();

    // Fix mount issue - wait_ready fail ignored before command GO_IDLE_STATE
    if (!wait(500)) {
        CONSOLE("wait_ready fail ignored, card initialize continues");
    }

    CardType = 0;
    Stat = STA_NOINIT;
    _encrc = 1;
    if (_cmd(CMD0, 0) != 0x01) /* Put the card SPI/Idle state */
        fail("IDLE failed");
    end();
    //HAL_Delay(100);

    {   
        /* пробуем включить контроль CRC - это есть только в esp32 */
        uint8_t r = scmd(CMD59, 0x1);
        if (r == 0x05)
            //old card maybe
            _encrc = 0;
        else
        if (r != 0x01)
            fail("CMD59(CRC_ON_OFF) failed: %u", r);
    }

    TMR(1000); /* Initialization timeout = 1 sec */
    if ((rcmd(CMD8, 0x1AA) == 0x01)) {//& 0x04 /* illegal cmd ? */) == 0 /* not illegal */) {
        /* Card id SDv2 */
        uint8_t rx[4];
        recv(rx, 4);
        end();
        if ((rx[2] != 0x01) || (rx[3] != 0xAA))
            fail("CMD8 failed: 0x%02x%02x%02x%02x", rx[0], rx[1], rx[2], rx[3]);
        
        /*
            это есть только в драйвере SD для esp32,
            но там так же присутствует попытка включения проверки CRC
        */
        if (rcmd(CMD58, 0) != 1)
            fail("READ_OCR fail");
        recv(rx, 4);
        end();
        if ((rx[1] & 0x10) == 0)
            fail("READ_OCR failed: 0x%02x%02x%02x%02x", rx[0], rx[1], rx[2], rx[3]);

        uint32_t p = 0x40000000;
        while (1) {
            uint8_t r = acmd(CMD41, p);
            if (r > 1)
                fail("CMD41 error: 0x%02x", r);
            if (r == 0)
                break;
            if (!TMROK)
                fail("CMD41 timeout");
            p = 0;
        }

        if (rcmd(CMD58, 0) > 0)
            fail("CMD58 error");
        recv(rx, 4);
        end();
        CardType = CT_SDC2;

        if (rx[0] & 0x40)
            CardType |= CT_BLOCK;
    }
    else {
        // Not SDv2 card
        uint8_t r;
        while (1) {
            r = acmd(CMD41, 0);
            if (r != 1)
                break;
            if (!TMROK)
                fail("ACMD41 timeout");
        }
        if (!r) {
            /* SDv1 (ACMD41(0)) */
            CardType = CT_SDC1;
        }
        else {
            while (1) {
                r = scmd(CMD1, 0);
                if (r > 1)
                    fail("CMD1 fail: 0x%02x", r);
                if (!r)
                    break;
                if (!TMROK)
                    fail("CMD1 timeout");
            }
            CardType = CT_MMC3;
        }
    }

    // APP_CLR_CARD_DETECT есть только в esp32
    if ((CardType == CT_MMC3) && (acmd(CMD42, 0) > 0))
        fail("CMD16(APP_CLR_CARD_DETECT) error");

    /* Set block length: 512 - for non-block addr type */
    if (!(CardType & CT_BLOCK) && (scmd(CMD16, 0x0200) > 0))
        fail("CMD16(SET_BLOCKLEN) error");
    end();

    if (CardType)			 /* OK */
        Stat &= ~STA_NOINIT; /* Clear STA_NOINIT flag */

    clk_chg(cperv); /* Set fast clock */

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE drv /* Physical drive number (0) */
) {
    if (drv)
        return STA_NOINIT; /* Supports only drive 0 */

    return Stat; /* Return disk status */
}

/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
    BYTE drv,	  /* Physical drive number (0) */
    BYTE *buff,	  /* Pointer to the data buffer to store read data */
    LBA_t sector, /* Start sector number (LBA) */
    UINT count	  /* Number of sectors to read (1..128) */
) {
    DWORD sect = (DWORD)sector;

    if (drv || !count)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check if drive is ready */

    if (!(CardType & CT_BLOCK))
        sect *= 512; /* LBA ot BA conversion (byte addressing cards) */

    if (count == 1) {									  /* Single sector read */
        if ((rcmd(CMD17, sect) == 0) /* READ_SINGLE_BLOCK */
            && recvblk(buff, 512)) {
            count = 0;
        }
    }
    else
    { /* Multiple sector read */
        if (rcmd(CMD18, sect) == 0) { /* READ_MULTIPLE_BLOCK */
            do
            {
                if (!recvblk(buff, 512))
                    break;
                buff += 512;
            } while (--count);
            _cmd(CMD12, 0); /* STOP_TRANSMISSION */
        }
    }
    end();

    return count ? RES_ERROR : RES_OK; /* Return result */
}

/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write(
    BYTE drv,		  /* Physical drive number (0) */
    const BYTE *buff, /* Ponter to the data to write */
    LBA_t sector,	  /* Start sector number (LBA) */
    UINT count		  /* Number of sectors to write (1..128) */
) {
    DWORD sect = (DWORD)sector;

    if (drv || !count)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check drive status */
    if (Stat & STA_PROTECT)
        return RES_WRPRT; /* Check write protect */

    if (!(CardType & CT_BLOCK))
        sect *= 512; /* LBA ==> BA conversion (byte addressing cards) */

    if (count == 1) {									  /* Single sector write */
        if ((rcmd(CMD24, sect) == 0) /* WRITE_BLOCK */
            && sendblk(buff, 0xFE)) {
            count = 0;
        }
    }
    else
    { /* Multiple sector write */
        if (CardType & CT_SDC)
            acmd(CMD23, count); /* Predefine number of sectors */
        if (rcmd(CMD25, sect) == 0) { /* WRITE_MULTIPLE_BLOCK */
            do
            {
                if (!sendblk(buff, 0xFC))
                    break;
                buff += 512;
            } while (--count);
            if (!sendblk(0, 0xFD))
                count = 1; /* STOP_TRAN token */
        }
    }
    end();

    return count ? RES_ERROR : RES_OK; /* Return result */
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(
    BYTE drv,  /* Physical drive number (0) */
    BYTE cmd,  /* Control command code */
    void *buff /* Pointer to the conrtol data */
) {
    DRESULT res;
    BYTE n, csd[16];
    DWORD st, ed, csize;
    LBA_t *dp;

    if (drv)
        return RES_PARERR; /* Check parameter */
    if (Stat & STA_NOINIT)
        return RES_NOTRDY; /* Check if drive is ready */

    res = RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC: /* Wait for end of internal write process of the drive */
        if (start())
            res = RES_OK;
        break;

    case GET_SECTOR_COUNT: /* Get drive capacity in unit of sector (DWORD) */
        if ((rcmd(CMD9, 0) == 0) && recvblk(csd, 16)) {
            if ((csd[0] >> 6) == 1) { /* SDC CSD ver 2 */
                csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
                *(LBA_t *)buff = csize << 10;
            }
            else
            { /* SDC CSD ver 1 or MMC */
                n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                *(LBA_t *)buff = csize << (n - 9);
            }
            res = RES_OK;
        }
        break;

    case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
        if (CardType & CT_SDC2) { /* SDC ver 2+ */
            if (acmd(CMD13, 0) == 0) { /* Read SD status */
                dummy(1);
                if (recvblk(csd, 16)) { /* Read partial block */
                    /* Purge trailing data */
                    dummy(64 - 16);
                    //for (n = 64 - 16; n; n--)
                    //    xchg_spi(0xFF); 
                    *(DWORD *)buff = 16UL << (csd[10] >> 4);
                    res = RES_OK;
                }
            }
        }
        else
        { /* SDC ver 1 or MMC */
            if ((rcmd(CMD9, 0) == 0) && recvblk(csd, 16)) { /* Read CSD */
                if (CardType & CT_SDC1)
                { /* SDC ver 1.XX */
                    *(DWORD *)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
                }
                else
                { /* MMC */
                    *(DWORD *)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                }
                res = RES_OK;
            }
        }
        break;

    case CTRL_TRIM: /* Erase a block of sectors (used when _USE_ERASE == 1) */
        if (!(CardType & CT_SDC))
            break; /* Check if the card is SDC */
        if (disk_ioctl(drv, MMC_GET_CSD, csd))
            break; /* Get CSD */
        if (!(csd[10] & 0x40))
            break; /* Check if ERASE_BLK_EN = 1 */
        dp = buff;
        st = (DWORD)dp[0];
        ed = (DWORD)dp[1]; /* Load sector block */
        if (!(CardType & CT_BLOCK)) {
            st *= 512;
            ed *= 512;
        }
        if (scmd(CMD32, st) == 0 && scmd(CMD33, ed) == 0 && scmd(CMD38, 0) == 0 && wait(30000)) {				  /* Erase sector block */
            res = RES_OK; /* FatFs does not check result of this command */
        }
        break;

        /* Following commands are never used by FatFs module */

    case MMC_GET_TYPE: /* Get MMC/SDC type (BYTE) */
        *(BYTE *)buff = CardType;
        res = RES_OK;
        break;

    case MMC_GET_CSD: /* Read CSD (16 bytes) */
        if (rcmd(CMD9, 0) == 0 && recvblk((BYTE *)buff, 16)) { /* READ_CSD */
            res = RES_OK;
        }
        break;

    case MMC_GET_CID: /* Read CID (16 bytes) */
        if (rcmd(CMD10, 0) == 0 && recvblk((BYTE *)buff, 16)) { /* READ_CID */
            res = RES_OK;
        }
        break;

    case MMC_GET_OCR: /* Read OCR (4 bytes) */
        if (rcmd(CMD58, 0) == 0) { /* READ_OCR */
            recv(buff, 4);
            res = RES_OK;
        }
        break;

    case MMC_GET_SDSTAT: /* Read SD status (64 bytes) */
        if (acmd(CMD13, 0) == 0) { /* SD_STATUS */
            dummy(1);
            if (recvblk((BYTE *)buff, 64))
                res = RES_OK;
        }
        break;

    default:
        res = RES_PARERR;
    }

    end();

    return res;
}
