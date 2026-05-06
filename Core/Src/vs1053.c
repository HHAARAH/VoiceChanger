#include "vs1053.h"

extern SPI_HandleTypeDef hspi1;

/* ================================================================
   Chip select helpers
   ================================================================ */
static inline void XCS_Low(void)  { HAL_GPIO_WritePin(XCS_PORT, XCS_PIN, GPIO_PIN_RESET); }
static inline void XCS_High(void) { HAL_GPIO_WritePin(XCS_PORT, XCS_PIN, GPIO_PIN_SET); }
static inline void XDCS_Low(void)  { HAL_GPIO_WritePin(XDCS_PORT, XDCS_PIN, GPIO_PIN_RESET); }
static inline void XDCS_High(void) { HAL_GPIO_WritePin(XDCS_PORT, XDCS_PIN, GPIO_PIN_SET); }

/* ================================================================
   Low-level SPI
   ================================================================ */
static uint8_t SPI_TransferByte(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, 100);
    return rx;
}

/* ================================================================
   DREQ handling
   ================================================================ */
uint8_t VS1053_CheckDREQ(void)
{
    return (HAL_GPIO_ReadPin(DREQ_PORT, DREQ_PIN) == GPIO_PIN_SET);
}

static void VS1053_WaitDREQ(void)
{
    uint32_t timeout = 100000;
    while (!VS1053_CheckDREQ() && --timeout);
}

/* ================================================================
   Standard SCI register access (waits for DREQ)
   ================================================================ */
void Mp3WriteRegister(uint8_t addr, uint8_t high, uint8_t low)
{
    VS1053_WaitDREQ();
    XCS_Low();
    SPI_TransferByte(VS_WRITE_COMMAND);
    SPI_TransferByte(addr);
    SPI_TransferByte(high);
    SPI_TransferByte(low);
    XCS_High();
}

uint16_t Mp3ReadRegister(uint8_t addr)
{
    uint16_t val;
    VS1053_WaitDREQ();
    XCS_Low();
    SPI_TransferByte(VS_READ_COMMAND);
    SPI_TransferByte(addr);
    val  = (uint16_t)SPI_TransferByte(0xFF) << 8;
    val |= SPI_TransferByte(0xFF);
    XCS_High();
    return val;
}

/* ================================================================
   Recording-only register access (no DREQ wait)
   ================================================================ */
void RecWriteRegister(uint8_t addr, uint8_t high, uint8_t low)
{
    XCS_Low();
    SPI_TransferByte(VS_WRITE_COMMAND);
    SPI_TransferByte(addr);
    SPI_TransferByte(high);
    SPI_TransferByte(low);
    XCS_High();
}

uint16_t RecReadRegister(uint8_t addr)
{
    uint16_t val;
    XCS_Low();
    SPI_TransferByte(VS_READ_COMMAND);
    SPI_TransferByte(addr);
    val  = (uint16_t)SPI_TransferByte(0xFF) << 8;
    val |= SPI_TransferByte(0xFF);
    XCS_High();
    return val;
}

/* ================================================================
   Reset & init
   ================================================================ */
void VS1053_Reset(void)
{
    HAL_GPIO_WritePin(XRST_PORT, XRST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    XCS_High();
    XDCS_High();
    HAL_GPIO_WritePin(XRST_PORT, XRST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
    VS1053_WaitDREQ();
}

void VS1053_SoftReset(void)
{
    Mp3WriteRegister(SPI_MODE, 0x08, 0x04);
    HAL_Delay(10);
    VS1053_WaitDREQ();

    Mp3WriteRegister(SPI_CLOCKF, 0x98, 0x00);
    VS1053_SetVolume(0x10, 0x10);
}

void VS1053_Init(void)
{
    VS1053_Reset();
    VS1053_SoftReset();
}

/* ================================================================
   Sine test
   ================================================================ */
void VS1053_SineTest(void)
{
    Mp3WriteRegister(SPI_MODE, 0x08, 0x20);
    VS1053_WaitDREQ();

    XDCS_Low();
    uint8_t start[] = {0x53, 0xEF, 0x6E, 0x44, 0, 0, 0, 0};
    for (int i = 0; i < 8; i++) SPI_TransferByte(start[i]);
    XDCS_High();
    HAL_Delay(500);

    XDCS_Low();
    uint8_t exit[] = {0x45, 0x78, 0x69, 0x74, 0, 0, 0, 0};
    for (int i = 0; i < 8; i++) SPI_TransferByte(exit[i]);
    XDCS_High();

    VS1053_SoftReset();
}

/* ================================================================
   Generic playback (memory buffer)
   ================================================================ */
void VS1053_PlayMP3(const uint8_t *data, uint32_t size)
{
    uint32_t pos = 0;
    while (pos < size) {
        if (!VS1053_CheckDREQ()) continue;
        uint32_t chunk = (size - pos < 32) ? size - pos : 32;
        XDCS_Low();
        for (uint32_t i = 0; i < chunk; i++) SPI_TransferByte(data[pos + i]);
        XDCS_High();
        pos += chunk;
    }
    /* Flush decoder with 2048 zero bytes */
    for (int i = 0; i < 64; i++) {
        if (!VS1053_CheckDREQ()) continue;
        XDCS_Low();
        for (int j = 0; j < 32; j++) SPI_TransferByte(0x00);
        XDCS_High();
    }
}

/* ================================================================
   PCM Recording
   ================================================================ */
void VS1053_StartPCMRecording(uint16_t sampleRate)
{
    RecWriteRegister(SPI_AICTRL0, sampleRate >> 8, sampleRate & 0xFF);
    RecWriteRegister(SPI_AICTRL1, 0x00, 0x00);
    RecWriteRegister(SPI_AICTRL2, 0x10, 0x00);
    RecWriteRegister(SPI_AICTRL3, 0x00, 0x01);  /* stereo, linear PCM */
    RecWriteRegister(SPI_MODE, 0x18, 0x00);      /* SM_SDINEW | SM_ADPCM, onboard mic */
    HAL_Delay(50);
}

void VS1053_StopRecording(void)
{
    RecWriteRegister(SPI_MODE, 0x08, 0x04);  /* SM_SDINEW | SM_RESET */
    HAL_Delay(10);
}
