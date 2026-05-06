#include "app_ui.h"
#include "vs1053.h"
#include <stdio.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;

static void Draw_Player_Overlay(void);
static void Draw_File_Page(void);
static void Draw_Record_Page(void);
static void Draw_Save_Overlay(void);
static void WriteWAVHeader(FIL *fil, uint16_t sampleRate, uint8_t channels, uint32_t dataSize);

App_State current_state = {0};

/* ================================================================
   Drawing helpers
   ================================================================ */
static void Draw_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const char *text, uint16_t color)
{
    LCD_Clear(x, y, w, h, color);
    uint16_t tw = strlen(text) * 8;
    LCD_DrawString_Color(x + (w - tw)/2, y + (h - 12)/2, text, color, BLACK);
}

static void Draw_Rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    LCD_DrawLine(x, y, x + w - 1, y, color);
    LCD_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    LCD_DrawLine(x, y, x, y + h - 1, color);
    LCD_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

/* ================================================================
   Pages
   ================================================================ */
void Draw_Main_Page(void)
{
    LCD_Clear(0, 0, 240, 320, WHITE);
    LCD_DrawString(60, 20, "Voice Changer");
    Draw_Button(20, 50,  200, 50, "Character", BLUE);
    Draw_Button(20, 120, 200, 50, "Record",    GREEN);
    Draw_Button(20, 190, 200, 50, "File",      YELLOW);
}

static void Draw_Char_Page(void)
{
    LCD_Clear(0, 0, 240, 320, WHITE);
    LCD_DrawString(70, 20, "Select Role");
    Draw_Button(20, 50,  200, 40, "Normal", CYAN);
    Draw_Button(20, 100, 200, 40, "Cat",    CYAN);
    Draw_Button(20, 150, 200, 40, "Dog",    CYAN);
    Draw_Button(20, 200, 200, 40, "Bird",   CYAN);

    if (current_state.selected_char != CHAR_NONE) {
        uint16_t y = 50 + (current_state.selected_char - 1) * 50;
        Draw_Rectangle(19, y-1, 202, 42, RED);
    }
    Draw_Button(180, 260, 50, 40, "Back", GREY);
}

static void Draw_Record_Page(void)
{
    LCD_Clear(0, 0, 240, 320, WHITE);
    LCD_DrawString(50, 20, "Audio Recording");
    if (current_state.is_recording) {
        Draw_Button(60, 140, 120, 120, "STOP", RED);
        LCD_DrawString(60, 100, "Recording...");
    } else {
        Draw_Button(60, 140, 120, 120, "START", GREEN);
        LCD_DrawString(40, 100, "Ready to Record");
    }
    Draw_Button(170, 270, 60, 40, "Back", GREY);
}

void Draw_File_Page(void)
{
    LCD_Clear(0, 0, 240, 320, WHITE);
    LCD_DrawString(60, 20, "File Manager");

    DIR dir;
    FILINFO fno;
    uint16_t file_idx = 0;
    uint16_t skip = current_state.file_page * FILES_PER_PAGE;
    current_state.file_count = 0;
    current_state.total_files = 0;

    if (f_opendir(&dir, "/") == FR_OK) {
        while (1) {
            if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0) break;
            if (fno.fattrib & AM_DIR) continue;
            current_state.total_files++;
            if (current_state.file_count < FILES_PER_PAGE && file_idx >= skip) {
                strncpy(current_state.file_list[current_state.file_count], fno.fname, 12);
                current_state.file_list[current_state.file_count][12] = '\0';
                char buf[20];
                sprintf(buf, "%d. %s", file_idx + 1, fno.fname);
                LCD_DrawString(20, 60 + current_state.file_count * 35, buf);
                current_state.file_count++;
            }
            file_idx++;
        }
        f_closedir(&dir);
    }

    uint16_t total = (current_state.total_files == 0) ? 1 :
                     (current_state.total_files + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
    Draw_Button(20,  270, 50, 30, "Prev",
                (current_state.file_page == 0) ? LIGHTGREY : BLUE);
    Draw_Button(95,  270, 50, 30, "Next",
                (current_state.file_page >= total - 1) ? LIGHTGREY : BLUE);
    char page_txt[10];
    sprintf(page_txt, "%d/%d", current_state.file_page + 1, total);
    LCD_DrawString(155, 275, page_txt);
    Draw_Button(170, 270, 50, 30, "Back", GREY);
}

/* ================================================================
   WAV header (PCM)
   ================================================================ */
static void WriteWAVHeader(FIL *fil, uint16_t sampleRate, uint8_t channels, uint32_t dataSize)
{
    uint8_t hdr[44] = {0};
    uint16_t bitsPerSample = 16;
    uint16_t blockAlign = channels * (bitsPerSample / 8);
    uint32_t byteRate = sampleRate * blockAlign;
    uint32_t fileSize = dataSize + 36;

    /* RIFF */
    memcpy(&hdr[0],  "RIFF", 4);
    hdr[4]  = (uint8_t)(fileSize);
    hdr[5]  = (uint8_t)(fileSize >> 8);
    hdr[6]  = (uint8_t)(fileSize >> 16);
    hdr[7]  = (uint8_t)(fileSize >> 24);
    memcpy(&hdr[8],  "WAVE", 4);
    /* fmt */
    memcpy(&hdr[12], "fmt ", 4);
    hdr[16] = 16; hdr[17] = 0; hdr[18] = 0; hdr[19] = 0;
    hdr[20] = 0x01; hdr[21] = 0x00;  /* PCM */
    hdr[22] = channels; hdr[23] = 0;
    hdr[24] = (uint8_t)(sampleRate);
    hdr[25] = (uint8_t)(sampleRate >> 8);
    hdr[26] = 0; hdr[27] = 0;
    hdr[28] = (uint8_t)(byteRate);
    hdr[29] = (uint8_t)(byteRate >> 8);
    hdr[30] = (uint8_t)(byteRate >> 16);
    hdr[31] = (uint8_t)(byteRate >> 24);
    hdr[32] = blockAlign; hdr[33] = 0;
    hdr[34] = bitsPerSample; hdr[35] = 0;
    /* data */
    memcpy(&hdr[36], "data", 4);
    hdr[40] = (uint8_t)(dataSize);
    hdr[41] = (uint8_t)(dataSize >> 8);
    hdr[42] = (uint8_t)(dataSize >> 16);
    hdr[43] = (uint8_t)(dataSize >> 24);

    UINT bw;
    f_write(fil, hdr, 44, &bw);
}

/* ================================================================
   Overlays
   ================================================================ */
static void Draw_Save_Overlay(void)
{
    LCD_Clear(30, 90, 180, 140, LIGHTGREY);
    Draw_Rectangle(30, 90, 180, 140, BLACK);
    LCD_DrawString(50, 110, "Save Recording?");
    Draw_Button(40,  170, 70, 40, "Yes", GREEN);
    Draw_Button(130, 170, 70, 40, "No",  RED);
}

static void Draw_Player_Overlay(void)
{
    LCD_Clear(10, 70, 220, 180, WHITE);
    Draw_Rectangle(10, 70, 220, 180, BLACK);
    LCD_DrawString(50, 90, current_state.current_file);

    if (current_state.is_playing) {
        LCD_DrawString(80, 120, "Playing...");
        LCD_Clear(20, 150, 200, 12, GREY);
        uint16_t w = (200 * current_state.play_progress) / 100;
        LCD_Clear(20, 150, w, 12, BLUE);
        Draw_Button(80, 180, 80, 40, "Pause", RED);
    } else {
        Draw_Button(20,  180, 60, 40, "Play",  GREEN);
        Draw_Button(90,  180, 60, 40, "Del",   RED);
        Draw_Button(160, 180, 60, 40, "Close", YELLOW);
    }
}

/* ================================================================
   Voice effect
   ================================================================ */
static void ApplyVoiceEffect(void)
{
    uint16_t speed = 1;
    switch (current_state.selected_char) {
        case CHAR_CAT:    speed = 2; break;
        case CHAR_DOG:    speed = 1; break;
        case CHAR_BIRD:   speed = 3; break;
        case CHAR_NORMAL: speed = 1; break;
        default: speed = 1; break;
    }

    if (speed == 1) return;

    Mp3WriteRegister(SPI_WRAMADDR, 0x1E, 0x04);
    Mp3WriteRegister(SPI_WRAM, (speed >> 8) & 0xFF, speed & 0xFF);
    //PlaySpeed in DSP RAM
    uint8_t bass = 0, freq = 0;
    switch(current_state.selected_char) {
        case CHAR_CAT:   bass = 0x4A; freq = 0x00; break;
        case CHAR_DOG:   bass = 0x00; freq = 0xA8; break;
        case CHAR_BIRD:  bass = 0x6C; freq = 0x00; break;
        default: break;
    }
    Mp3WriteRegister(SPI_BASS, bass, freq);
    //[ Treble_Amp (4bit) | Treble_Freq (4bit) | Bass_Amp (4bit) | Bass_Freq (4bit) ]
}
/* ================================================================
   Audio task (playback + recording)
   ================================================================ */
void AudioTask(void)
{
    /* ---- Playback ---- */
    if (current_state.is_playing) {
        for (int batch = 0; batch < 8; batch++) {
            if (!VS1053_CheckDREQ()) break;
            if (current_state.play_buf_idx >= current_state.play_buf_len) {
                UINT br = 0;
                FRESULT res = f_read(&current_state.play_fil, current_state.play_buffer, 512, &br);
                if (res != FR_OK || br == 0) {
                    /* EOF – flush decoder with zeros */
                    for (int i = 0; i < 16; i++) {
                        if (!VS1053_CheckDREQ()) continue;
                        HAL_GPIO_WritePin(XDCS_PORT, XDCS_PIN, GPIO_PIN_RESET);
                        uint8_t z[32] = {0};
                        HAL_SPI_Transmit(&hspi1, z, 32, 50);
                        HAL_GPIO_WritePin(XDCS_PORT, XDCS_PIN, GPIO_PIN_SET);
                    }
                    current_state.is_playing = 0;
                    current_state.play_progress = 0;
                    f_close(&current_state.play_fil);
                    Draw_Player_Overlay();
                    printf("Playback Finished.\n");
                    return;
                }
                current_state.play_buf_len = br;
                current_state.play_buf_idx = 0;
            }

            uint16_t avail = current_state.play_buf_len - current_state.play_buf_idx;
            uint16_t chunk = (avail > 32) ? 32 : avail;

            HAL_GPIO_WritePin(XDCS_PORT, XDCS_PIN, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi1, &current_state.play_buffer[current_state.play_buf_idx], chunk, 50);
            HAL_GPIO_WritePin(XDCS_PORT, XDCS_PIN, GPIO_PIN_SET);

            current_state.play_buf_idx  += chunk;
            current_state.play_bytes_read += chunk;
        }

        current_state.play_progress = (current_state.play_bytes_read * 100) / current_state.play_file_size;
        uint16_t w = (200 * current_state.play_progress) / 100;
        LCD_Clear(20, 150, w, 12, BLUE);
    }

    /* ---- Recording ---- */
    if (current_state.is_recording) {
        uint16_t hdat1 = RecReadRegister(SPI_HDAT1);
        uint16_t words = ((hdat1 >> 8) > (hdat1 & 0xFF)) ? (hdat1 >> 8) : (hdat1 & 0xFF);
        if (words > 1024) words = 1024;
        for (uint16_t i = 0; i < words; i++) {
            uint16_t w = RecReadRegister(SPI_HDAT0);
            current_state.rec_buffer[current_state.rec_buf_len++] = (uint8_t)(w);
            current_state.rec_buffer[current_state.rec_buf_len++] = (uint8_t)(w >> 8);
        }
        if (current_state.rec_buf_len >= 4096) {
            UINT bw;
            f_write(&current_state.rec_fil, current_state.rec_buffer, 4096, &bw);
            current_state.rec_data_bytes += 4096;
            current_state.rec_buf_len = 0;
        }
    }
}

/* ================================================================
   Playback control
   ================================================================ */

static void StartPlayback(const char *filename)
{
    current_state.is_playing = 0;
    f_close(&current_state.play_fil);
    VS1053_SoftReset();
    while(!VS1053_CheckDREQ());

    if (f_open(&current_state.play_fil, filename, FA_READ) != FR_OK) {
        printf("Failed to open: %s\n", filename);
        return;
    }
    current_state.play_file_size = f_size(&current_state.play_fil);
    current_state.play_bytes_read = 0;
    current_state.play_buf_idx = 0;
    current_state.play_buf_len = 0;
    current_state.play_progress = 0;

    ApplyVoiceEffect();
    current_state.is_playing = 1;
    printf("Start Playing: %s (%lu B)\n", filename, current_state.play_file_size);
}
/* ================================================================
   Touch handlers
   ================================================================ */
static void Handle_Main_Touch(uint16_t x, uint16_t y)
{
    if      (y > 50  && y < 100) { current_state.current_page = PAGE_CHAR;   Draw_Char_Page(); }
    else if (y > 120 && y < 170) { current_state.current_page = PAGE_RECORD; Draw_Record_Page(); }
    else if (y > 190 && y < 240) { current_state.current_page = PAGE_FILE;   Draw_File_Page(); }
}

static void Handle_Char_Touch(uint16_t x, uint16_t y)
{
    if (x > 180 && y > 260) { current_state.current_page = PAGE_MAIN; Draw_Main_Page(); return; }
    if (x > 20 && x < 220) {
        if      (y > 50  && y < 90)  current_state.selected_char = CHAR_NORMAL;
        else if (y > 100 && y < 140) current_state.selected_char = CHAR_CAT;
        else if (y > 150 && y < 190) current_state.selected_char = CHAR_DOG;
        else if (y > 200 && y < 240) current_state.selected_char = CHAR_BIRD;
        ApplyVoiceEffect();
        Draw_Char_Page();
    }
}

static void Handle_Record_Touch(uint16_t x, uint16_t y)
{
    if (x > 170 && y > 270) { current_state.current_page = PAGE_MAIN; Draw_Main_Page(); return; }

    if (x > 60 && x < 180 && y > 140 && y < 260) {
        current_state.is_recording ^= 1;
        if (!current_state.is_recording) {
            uint16_t hdat1 = RecReadRegister(SPI_HDAT1);
            uint16_t words = ((hdat1 >> 8) > (hdat1 & 0xFF)) ? (hdat1 >> 8) : (hdat1 & 0xFF);
            while (words--) {
                uint16_t w = RecReadRegister(SPI_HDAT0);
                current_state.rec_buffer[current_state.rec_buf_len++] = (uint8_t)w;
                current_state.rec_buffer[current_state.rec_buf_len++] = (uint8_t)(w >> 8);
            }
            if (current_state.rec_buf_len > 0) {
                UINT bw;
                f_write(&current_state.rec_fil, current_state.rec_buffer, current_state.rec_buf_len, &bw);
                current_state.rec_data_bytes += current_state.rec_buf_len;
                current_state.rec_buf_len = 0;
            }
            VS1053_StopRecording();
            current_state.active_overlay = OVERLAY_SAVE;
            Draw_Save_Overlay();
        }
        else {
            char fname[13];
            int idx = 0;
            for (int i = 0; i <= 9999; i++) {
                sprintf(fname, "REC%04d.WAV", i);
                if (f_open(&current_state.rec_fil, fname, FA_READ) != FR_OK) break;
                f_close(&current_state.rec_fil);
                idx = i + 1;
            }
            sprintf(fname, "REC%04d.WAV", idx);
            f_open(&current_state.rec_fil, fname, FA_CREATE_ALWAYS | FA_WRITE);
            f_lseek(&current_state.rec_fil, 44);
            current_state.rec_buf_len = 0;
            current_state.rec_data_bytes = 0;
            current_state.rec_sample_rate = 16000;
            VS1053_StartPCMRecording(current_state.rec_sample_rate);
            Draw_Record_Page();
            printf("Recording: %s\n", fname);
        }
    }
}

static void Handle_File_Touch(uint16_t x, uint16_t y)
{
    /* Prev page */
    if (x > 20 && x < 70 && y > 270 && y < 300) {
        if (current_state.file_page > 0) { current_state.file_page--; Draw_File_Page(); }
        return;
    }
    /* Next page */
    if (x > 95 && x < 145 && y > 270 && y < 300) {
        uint16_t total = (current_state.total_files == 0) ? 1 :
                         (current_state.total_files + FILES_PER_PAGE - 1) / FILES_PER_PAGE;
        if (current_state.file_page < total - 1) { current_state.file_page++; Draw_File_Page(); }
        return;
    }
    /* Back */
    if (x > 170 && x < 220 && y > 270 && y < 300) {
        current_state.current_page = PAGE_MAIN; Draw_Main_Page(); return;
    }
    /* File list entries */
    for (uint8_t i = 0; i < current_state.file_count; i++) {
        uint16_t ly = 60 + i * 35;
        if (y > ly && y < ly + 30 && x > 20 && x < 220) {
            strcpy(current_state.current_file, current_state.file_list[i]);
            current_state.active_overlay = OVERLAY_PLAYER;
            current_state.is_playing = 0;
            current_state.play_progress = 0;
            Draw_Player_Overlay();
            return;
        }
    }
}

/* ================================================================
   Loop
   ================================================================ */
void UI_Init(void) { Draw_Main_Page(); }

void UI_Loop(void)
{
    strType_XPT2046_Coordinate coord;
    if (ucXPT2046_TouchFlag == 1) {
        ucXPT2046_TouchFlag = 0;
        if (!XPT2046_Get_TouchedPoint(&coord, &strXPT2046_TouchPara)) return;

        uint16_t x = coord.x, y = coord.y;

        if (current_state.active_overlay == OVERLAY_SAVE) {
            if (x > 40 && x < 110 && y > 170 && y < 210) {
                f_lseek(&current_state.rec_fil, 0);
                WriteWAVHeader(&current_state.rec_fil, current_state.rec_sample_rate, 2, current_state.rec_data_bytes);
                f_close(&current_state.rec_fil);
                printf("Saved: %lu bytes\n", current_state.rec_data_bytes);
                current_state.active_overlay = OVERLAY_NONE;
                current_state.current_page = PAGE_MAIN;
                Draw_Main_Page();
            }
            else if (x > 130 && x < 200 && y > 170 && y < 210) {
                current_state.active_overlay = OVERLAY_NONE;
                Draw_Record_Page();
            }
            return;
        }

        if (current_state.active_overlay == OVERLAY_PLAYER) {
            if (x > 160 && x < 220 && y > 180 && y < 220) {
                current_state.active_overlay = OVERLAY_NONE;
                current_state.is_playing = 0;
                f_close(&current_state.play_fil);
                VS1053_SoftReset();
                Draw_File_Page();
                return;
            }
            if (!current_state.is_playing && x > 20 && x < 80 && y > 180 && y < 220) {
                StartPlayback(current_state.current_file);
                Draw_Player_Overlay();
                return;
            }
            if (current_state.is_playing && x > 80 && x < 160 && y > 180 && y < 220) {
                current_state.is_playing = 0;
                Draw_Player_Overlay();
                printf("Paused.\n");
                return;
            }
            if (!current_state.is_playing && x > 90 && x < 150 && y > 180 && y < 220) {
                if (f_unlink(current_state.current_file) == FR_OK) {
                    printf("Deleted: %s\n", current_state.current_file);
                    current_state.active_overlay = OVERLAY_NONE;
                    Draw_File_Page();
                }
                return;
            }
            return;
        }

        switch (current_state.current_page) {
            case PAGE_MAIN:   Handle_Main_Touch(x, y);   break;
            case PAGE_CHAR:   Handle_Char_Touch(x, y);   break;
            case PAGE_RECORD: Handle_Record_Touch(x, y); break;
            case PAGE_FILE:   Handle_File_Touch(x, y);   break;
            default: break;
        }
    }

    AudioTask();
}
