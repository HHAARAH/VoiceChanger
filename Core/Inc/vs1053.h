#ifndef __VS1053_H__
#define __VS1053_H__

#include "main.h"
#include <stdint.h>

/* SCI commands */
#define VS_WRITE_COMMAND  0x02
#define VS_READ_COMMAND   0x03

/* SCI register addresses */
#define SPI_MODE          0x00
#define SPI_STATUS        0x01
#define SPI_BASS          0x02
#define SPI_CLOCKF        0x03
#define SPI_DECODE_TIME   0x04
#define SPI_AUDATA        0x05
#define SPI_WRAM          0x06
#define SPI_WRAMADDR      0x07
#define SPI_HDAT0         0x08
#define SPI_HDAT1         0x09
#define SPI_VOL           0x0B
#define SPI_AICTRL0       0x0C
#define SPI_AICTRL1       0x0D
#define SPI_AICTRL2       0x0E
#define SPI_AICTRL3       0x0F

/* MODE register bits */
#define SM_DIFF           0x0001
#define SM_RESET          0x0004
#define SM_CANCEL         0x0008
#define SM_TESTS          0x0020
#define SM_STREAM         0x0040
#define SM_SDINEW         0x0800
#define SM_ADPCM          0x1000
#define SM_LINE1          0x4000

/* Pin definitions */
#define XCS_PIN         GPIO_PIN_2
#define XCS_PORT        GPIOA
#define XDCS_PIN        GPIO_PIN_3
#define XDCS_PORT       GPIOA
#define XRST_PIN        GPIO_PIN_3
#define XRST_PORT       GPIOD
#define DREQ_PIN        GPIO_PIN_4
#define DREQ_PORT       GPIOC

/* Volume macro */
#define VS1053_SetVolume(left, right)  Mp3WriteRegister(SPI_VOL, (left), (right))

/* Functions */
void VS1053_Init(void);
void VS1053_Reset(void);
void VS1053_SoftReset(void);
void VS1053_SineTest(void);
void VS1053_PlayMP3(const uint8_t *data, uint32_t size);

void Mp3WriteRegister(uint8_t addr, uint8_t high, uint8_t low);
uint16_t Mp3ReadRegister(uint8_t addr);
uint8_t VS1053_CheckDREQ(void);

void RecWriteRegister(uint8_t addr, uint8_t high, uint8_t low);
uint16_t RecReadRegister(uint8_t addr);

void VS1053_StartPCMRecording(uint16_t sampleRate);
void VS1053_StopRecording(void);

#endif
