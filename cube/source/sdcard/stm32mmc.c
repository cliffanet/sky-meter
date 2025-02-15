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
#define SDCARD_PIN_CD	GPIOA, GPIO_PIN_3
#endif // HWVER

extern SPI_HandleTypeDef hspi1;
extern RTC_HandleTypeDef hrtc;

static void clk_chg(uint32_t BaudRatePrescaler) {
    WRITE_REG(
        hspi1.Instance->CR1,
        (READ_REG(hspi1.Instance->CR1) & ~SPI_CR1_BR_Msk) | (BaudRatePrescaler & SPI_CR1_BR_Msk)
    );
}
static void clk_slow() {
    clk_chg(SPI_BAUDRATEPRESCALER_256);
}

#if HWVER < 2
static void clk_fast() {
    clk_chg(SPI_BAUDRATEPRESCALER_8);
}

#define sdcard_on()
#define sdcard_off()

static void cs_hi() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
}
static void cs_lo() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
}
#else
static void clk_fast() {
    clk_chg(SPI_BAUDRATEPRESCALER_4);
}

static void sdcard_on() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
}
static void sdcard_off() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
}

static void cs_hi() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}
static void cs_lo() {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}
#endif // HWVER

#define TMR(wait)   uint32_t _tmr = HAL_GetTick() + wait;
#define TMROK       (_tmr > HAL_GetTick())


/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/
#include "../ff/diskio.h"


/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */
#define CMD59	(59)		/* CRC_ON_OFF */


/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC3		0x01		/* MMC ver 3 */
#define CT_MMC4		0x02		/* MMC ver 4+ */
#define CT_MMC		0x03		/* MMC */
#define CT_SDC1		0x04		/* SD ver 1 */
#define CT_SDC2		0x08		/* SD ver 2+ */
#define CT_SDC		0x0C		/* SD */
#define CT_BLOCK	0x10		/* Block addressing */


static volatile DSTATUS Stat = STA_NOINIT;	/* Physical drive status */
static BYTE CardType;	/* Card type flags */


DWORD get_fattime (void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (
            (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) ||
            (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        ) {
        return 0;
    }

	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)((uint16_t)(sDate.Year) + 2000 - 1980) << 25)
			| ((DWORD)sDate.Month << 21)
			| ((DWORD)sDate.Date << 16)
			| ((DWORD)sTime.Hours << 11)
			| ((DWORD)sTime.Minutes << 5)
			| ((DWORD)sTime.Seconds >> 1);
}

/*-----------------------------------------------------------------------*/
/* SPI controls (Platform dependent)                                     */
/*-----------------------------------------------------------------------*/


/* Exchange a byte */
static BYTE xchg_spi (
	BYTE dat	/* Data to send */
)
{
    BYTE rxDat;
    HAL_SPI_TransmitReceive(&hspi1, &dat, &rxDat, 1, 50);
    return rxDat;
}


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static int wait_ready (	/* 1:Ready, 0:Timeout */
	UINT wt			/* Timeout [ms] */
)
{
	BYTE d;


	TMR(wt);
	do {
		d = xchg_spi(0xFF);
		/* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
	//} while (d != 0xFF && TMROK);	/* Wait for card goes ready or timeout */
    } while (d == 0x00 && TMROK); // esp-32 version

    if (!d)
        CONSOLE("wait_ready(%u) fail", wt);

	//return (d == 0xFF) ? 1 : 0;
    return (d > 0x00) ? 1 : 0; // esp-32 version
}



/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static void deselect (void)
{
	cs_hi();		/* Set CS# high */
	xchg_spi(0xFF);	/* Dummy clock (force DO hi-z for multiple slave SPI) */

}



/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

static int select (void)	/* 1:OK, 0:Timeout */
{
	cs_lo();		/* Set CS# low */
	xchg_spi(0xFF);	/* Dummy clock (force DO enabled) */
	if (wait_ready(500)) return 1;	/* Wait for card ready */

	deselect();
	return 0;	/* Timeout */
}



/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/

static int rcvr_datablock (	/* 1:OK, 0:Error */
	BYTE *buff,				/* Data buffer */
	UINT btr				/* Data block length (byte) */
)
{
	BYTE token;


    TMR(2000);
	do {							/* Wait for DataStart token in timeout of 200ms */
		token = xchg_spi(0xFF);
		/* This loop will take a time. Insert rot_rdq() here for multitask envilonment. */
	} while ((token == 0xFF) && TMROK);
	if(token != 0xFE) return 0;		/* Function fails if invalid DataStart token or timeout */

	/* Store trailing data to the buffer */
    
    //for (UINT i = 0; i < btr; i++) {
    //    *(buff + i) = xchg_spi(0xFF);
    //}
    BYTE tx[btr];
    memset(tx, 0xff, sizeof(tx));
    HAL_SPI_TransmitReceive(&hspi1, tx, buff, btr, 100);

	xchg_spi(0xFF); xchg_spi(0xFF);			/* Discard CRC */

	return 1;						/* Function succeeded */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
static int xmit_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buff,		/* Ponter to 512 byte data to be sent */
	BYTE token				/* Token */
)
{
	BYTE resp;


	if (!wait_ready(500)) return 0;		/* Wait for card ready */

	xchg_spi(token);					/* Send token */
	if (token != 0xFD) {				/* Send data if token is other than StopTran */
        HAL_SPI_Transmit(&hspi1, buff, 512, 1000);
		xchg_spi(0xFF); xchg_spi(0xFF);	/* Dummy CRC */

		resp = xchg_spi(0xFF);				/* Receive data resp */
		if ((resp & 0x1F) != 0x05) return 0;	/* Function fails if the data packet was not accepted */
	}
	return 1;
}
#endif


/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/
char CRC7(const char* data, int length);

static BYTE send_cmd_ (	/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,			/* Command index */
	DWORD arg			/* Argument */
)
{
	BYTE res;

    for (int f = 0; f < 3; f++) {
        if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
            cmd &= 0x7F;
            res = send_cmd_(CMD55, 0);
            if (res > 1)
                return res;
            deselect();
            if (!select())
                return 0xFF;
        }

        /* Send command packet */
        uint8_t pkt[7] = {
            cmd | 0x40,
            (BYTE)(arg >> 24),
            (BYTE)(arg >> 16),
            (BYTE)(arg >> 8),
            (BYTE)arg,
            //cmd == CMD0 ? 0x95 : cmd == CMD8 ? 0x87 : 0x01,
            0,
            0xFF
        };
        pkt[5] = (CRC7((char*)pkt, 5) << 1) | 0x01;

        HAL_SPI_Transmit(&hspi1, pkt, (cmd == CMD12) ? 7 : 6, 100);

        /* Wait for response (10 bytes max) */
        for (int i = 0; i < 10; i++) {
            res = xchg_spi(0xFF);
            if (!(res & 0x80))
                break;
        }

        if (res == 0xFF) {
            CONSOLE("[%u] no token received", cmd);
            deselect();
            HAL_Delay(100);
            select();
            continue;
        } else
        if (res & 0x08) {
            CONSOLE("[%u] crc error", cmd);
            deselect();
            HAL_Delay(100);
            select();
            continue;
        } else
        if (res > 1) {
            CONSOLE("[%u] token error 0x%x", cmd, res);
            break;
        }

        break;
    }

    if (res == 0xFF)
        CONSOLE("Card Failed! cmd: [%u]", cmd);
    
    return res;
}

static BYTE ssend_cmd (	/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,			/* Command index */
	DWORD arg			/* Argument */
)
{
    if (!select())
        return 0xFF;
    
    return send_cmd_(cmd, arg);
}

static BYTE send_cmd_recv (	/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,			/* Command index */
	DWORD arg,			/* Argument */
    BYTE *rx,           /* Receive data */
    size_t sz           /* Size of recv */
)
{
    BYTE res = ssend_cmd(cmd, arg);

    if ((rx != NULL) && (sz > 0)) {
        BYTE tx[sz];
        memset(tx, 0xff, sizeof(tx));
        HAL_SPI_TransmitReceive(&hspi1, tx, rx, sz, 100);
    }
    deselect();

    return res;
}
static BYTE send_cmd (	/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,			/* Command index */
	DWORD arg			/* Argument */
)
{
    return send_cmd_recv(cmd, arg, NULL, 0);
}


/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE drv		/* Physical drive number (0) */
)
{
	if (drv) return STA_NOINIT;			/* Supports only drive 0 */

	if (Stat & STA_NODISK) return Stat;	/* Is card existing in the soket? */

	sdcard_on();

	clk_slow();
    
    cs_hi();
    for (uint8_t i = 0; i < 20; i++) {
        xchg_spi(0XFF);
    }

    // Fix mount issue - wait_ready fail ignored before command GO_IDLE_STATE
    cs_lo();
    if(!wait_ready(500)){
        CONSOLE("wait_ready fail ignored, card initialize continues");
    }

    CardType = 0;
    Stat = STA_NOINIT;
    if (send_cmd_(CMD0, 0) != 1) { /* Put the card SPI/Idle state */
        deselect();
        clk_fast();
        CONSOLE("IDLE failed");
        return Stat;
    }		
    deselect();

    TMR(1000);  						/* Initialization timeout = 1 sec */
	BYTE ocr[4];
    if (send_cmd_recv(CMD8, 0x1AA, ocr, 4) == 1) {	/* SDv2? */
        if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* Is the card supports vcc of 2.7-3.6V? */
            if ((send_cmd_recv(CMD58, 0, ocr, 4) != 1) || !(ocr[1] & (1 << 4))) {
                clk_fast();
                CONSOLE("READ_OCR failed: 0x%02x%02x%02x%02x", ocr[0], ocr[1], ocr[2], ocr[3]);
                return Stat;
            }

            // без отключения CRC не работает ACMD41 для карт sandisk, помеченных как SDHC(3).
            // При этом нет ошибок CRC, а карту всё-таки можно запустить, если в debug-режиме
            // выполнять команды пошагово
            BYTE r = send_cmd(CMD59, 0);
            if (r != 1)
                CONSOLE("CRC on fail: %d", r);

            while (((r = send_cmd(ACMD41, 0x40100000)) == 1) && TMROK) ; //1UL << 30) && TMROK) ;	/* Wait for end of initialization with ACMD41(HCS) */
            if (r)
                CONSOLE("APP_OP_COND failed: %u", r);

            if (send_cmd_recv(CMD58, 0, ocr, 4) == 0) {		/* Check CCS bit in the OCR */
                CardType = (ocr[0] & 0x40) ? CT_SDC2 | CT_BLOCK : CT_SDC2;	/* Card id SDv2 */
            }

            while (send_cmd(CMD1, 0) && TMROK) ;	/* Wait for end of initialization */
        }
    }
    else {	/* Not SDv2 card */
        BYTE cmd;
        if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
            CardType = CT_SDC1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
        } else {
            CardType = CT_MMC3; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
        }
        while (TMROK && send_cmd(cmd, 0)) ;		/* Wait for end of initialization */
        if (!TMROK || send_cmd(CMD16, 512) != 0)	/* Set block length: 512 */
            CardType = 0;
    }
    
	if (CardType)			/* OK */
		Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT flag */
	
    clk_fast();			/* Set fast clock */

	return Stat;
}

void disk_poweroff() {
	sdcard_off();
}



/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv		/* Physical drive number (0) */
)
{
	if (drv) return STA_NOINIT;		/* Supports only drive 0 */

	return Stat;	/* Return disk status */
}



/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,		/* Physical drive number (0) */
	BYTE *buff,		/* Pointer to the data buffer to store read data */
	LBA_t sector,	/* Start sector number (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	DWORD sect = (DWORD)sector;


	if (drv || !count) return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	if (!(CardType & CT_BLOCK)) sect *= 512;	/* LBA ot BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector read */
		if ((ssend_cmd(CMD17, sect) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512)) {
			count = 0;
		}
	}
	else {				/* Multiple sector read */
		if (ssend_cmd(CMD18, sect) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}



/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive number (0) */
	const BYTE *buff,	/* Ponter to the data to write */
	LBA_t sector,		/* Start sector number (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	DWORD sect = (DWORD)sector;


	if (drv || !count) return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check drive status */
	if (Stat & STA_PROTECT) return RES_WRPRT;	/* Check write protect */

	if (!(CardType & CT_BLOCK)) sect *= 512;	/* LBA ==> BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector write */
		if ((ssend_cmd(CMD24, sect) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE)) {
			count = 0;
		}
	}
	else {				/* Multiple sector write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);	/* Predefine number of sectors */
		if (ssend_cmd(CMD25, sect) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD)) count = 1;	/* STOP_TRAN token */
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive number (0) */
	BYTE cmd,		/* Control command code */
	void *buff		/* Pointer to the conrtol data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD st, ed, csize;
	LBA_t *dp;


	if (drv) return RES_PARERR;					/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	res = RES_ERROR;

	switch (cmd) {
	case CTRL_SYNC :		/* Wait for end of internal write process of the drive */
		if (select()) res = RES_OK;
		break;

	case GET_SECTOR_COUNT :	/* Get drive capacity in unit of sector (DWORD) */
		if ((ssend_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
			if ((csd[0] >> 6) == 1) {	/* SDC CSD ver 2 */
				csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
				*(LBA_t*)buff = csize << 10;
			} else {					/* SDC CSD ver 1 or MMC */
				n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(LBA_t*)buff = csize << (n - 9);
			}
			res = RES_OK;
		}
		break;

	case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
		if (CardType & CT_SDC2) {	/* SDC ver 2+ */
			if (ssend_cmd(ACMD13, 0) == 0) {	/* Read SD status */
				xchg_spi(0xFF);
				if (rcvr_datablock(csd, 16)) {				/* Read partial block */
					for (n = 64 - 16; n; n--) xchg_spi(0xFF);	/* Purge trailing data */
					*(DWORD*)buff = 16UL << (csd[10] >> 4);
					res = RES_OK;
				}
			}
		} else {					/* SDC ver 1 or MMC */
			if ((ssend_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {	/* Read CSD */
				if (CardType & CT_SDC1) {	/* SDC ver 1.XX */
					*(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				} else {					/* MMC */
					*(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
				}
				res = RES_OK;
			}
		}
		break;

	case CTRL_TRIM :	/* Erase a block of sectors (used when _USE_ERASE == 1) */
		if (!(CardType & CT_SDC)) break;				/* Check if the card is SDC */
		if (disk_ioctl(drv, MMC_GET_CSD, csd)) break;	/* Get CSD */
		if (!(csd[10] & 0x40)) break;					/* Check if ERASE_BLK_EN = 1 */
		dp = buff; st = (DWORD)dp[0]; ed = (DWORD)dp[1];	/* Load sector block */
		if (!(CardType & CT_BLOCK)) {
			st *= 512; ed *= 512;
		}
		if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0 && wait_ready(30000)) {	/* Erase sector block */
			res = RES_OK;	/* FatFs does not check result of this command */
		}
		break;

	/* Following commands are never used by FatFs module */

	case MMC_GET_TYPE:		/* Get MMC/SDC type (BYTE) */
		*(BYTE*)buff = CardType;
		res = RES_OK;
		break;

	case MMC_GET_CSD:		/* Read CSD (16 bytes) */
		if (ssend_cmd(CMD9, 0) == 0 && rcvr_datablock((BYTE*)buff, 16)) {	/* READ_CSD */
			res = RES_OK;
		}
		break;

	case MMC_GET_CID:		/* Read CID (16 bytes) */
		if (ssend_cmd(CMD10, 0) == 0 && rcvr_datablock((BYTE*)buff, 16)) {	/* READ_CID */
			res = RES_OK;
		}
		break;

	case MMC_GET_OCR:		/* Read OCR (4 bytes) */
		if (ssend_cmd(CMD58, 0) == 0) {	/* READ_OCR */
			for (n = 0; n < 4; n++) *(((BYTE*)buff) + n) = xchg_spi(0xFF);
			res = RES_OK;
		}
		break;

	case MMC_GET_SDSTAT:	/* Read SD status (64 bytes) */
		if (ssend_cmd(ACMD13, 0) == 0) {	/* SD_STATUS */
			xchg_spi(0xFF);
			if (rcvr_datablock((BYTE*)buff, 64)) res = RES_OK;
		}
		break;

	default:
		res = RES_PARERR;
	}

	deselect();

	return res;
}
