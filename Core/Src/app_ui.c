#include "app_ui.h"
#include <stdio.h>
#include <string.h>

App_State current_state = {
    .current_page = PAGE_MAIN,
    .active_overlay = OVERLAY_NONE,
    .selected_char = CHAR_NONE,
    .is_recording = 0,
    .is_playing = 0,
    .play_progress = 0,
    .current_file = "",
    .file_count = 0
};

// ==================================

static void Draw_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* text, uint16_t color)
{
    LCD_Clear(x, y, w, h, color);
    uint16_t len = strlen(text) * 8;
    LCD_DrawString_Color(x + (w - len)/2, y + (h - 12)/2, text, color, BLACK);
}

static void Draw_Rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    LCD_DrawLine(x, y, x + w - 1, y, color);
    LCD_DrawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    LCD_DrawLine(x, y, x, y + h - 1, color);
    LCD_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

// ==================================

void Draw_Main_Page(void)
{
    LCD_Clear(0, 0, 240, 320, WHITE);
    LCD_DrawString(60, 20, "Voice Changer");
    Draw_Button(20, 50, 200, 50, "Character", BLUE);
    Draw_Button(20, 120, 200, 50, "Record", GREEN);
    Draw_Button(20, 190, 200, 50, "File", YELLOW);
}

static void Draw_Char_Page(void)
{
    LCD_Clear(0, 0, 240, 320, WHITE);
    LCD_DrawString(70, 20, "Select Role");
    Draw_Button(20, 60, 200, 50, "Cat", CYAN);
    Draw_Button(20, 130, 200, 50, "Dog", CYAN);
    Draw_Button(20, 200, 200, 50, "Bird", CYAN);
    
    if (current_state.selected_char != CHAR_NONE) {
        uint16_t y = 60 + (current_state.selected_char - 1) * 70;
        Draw_Rectangle(19, y-1, 202, 52, RED);
    }

    Draw_Button(170, 270, 60, 40, "Back", GREY);
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
    FRESULT res;
    current_state.file_count = 0;

    res = f_opendir(&dir, "/");
    if (res == FR_OK)
    {
        while (current_state.file_count < 6)
        {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;

            if (fno.fattrib & AM_DIR) continue;

            strncpy(current_state.file_list[current_state.file_count], fno.fname, 12);
            current_state.file_list[current_state.file_count][12] = '\0';

            char display_str[20];
            sprintf(display_str, "%d. %s", current_state.file_count + 1, fno.fname);
            LCD_DrawString(20, 60 + current_state.file_count * 35, display_str);

            current_state.file_count++;
        }
        f_closedir(&dir);
    }

    if (current_state.file_count == 0) {
        LCD_DrawString(50, 150, "No Files Found");
    }

    Draw_Button(170, 270, 60, 40, "Back", GREY);
}

// ==================================

static void Draw_Save_Overlay(void)
{
    LCD_Clear(30, 90, 180, 140, LIGHTGREY);
    Draw_Rectangle(30, 90, 180, 140, BLACK);
    LCD_DrawString(50, 110, "Save Recording?");

    Draw_Button(40, 170, 70, 40, "Yes", GREEN);
    Draw_Button(130, 170, 70, 40, "No", RED);
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
    }
    else {
        Draw_Button(20, 180, 60, 40, "Play", GREEN);
        Draw_Button(90, 180, 60, 40, "Del", RED);
        Draw_Button(160, 180, 60, 40, "Close", YELLOW);
    }
}

// ==================================

static void Handle_Main_Touch(uint16_t x, uint16_t y)
{
    if (y > 50 && y < 100) { current_state.current_page = PAGE_CHAR; Draw_Char_Page(); }
    else if (y > 120 && y < 170) { current_state.current_page = PAGE_RECORD; Draw_Record_Page(); }
    else if (y > 190 && y < 240) { current_state.current_page = PAGE_FILE; Draw_File_Page(); }
}

static void Handle_Char_Touch(uint16_t x, uint16_t y)
{
    if (x > 170 && y > 270) { current_state.current_page = PAGE_MAIN; Draw_Main_Page(); return; }

    if (x > 20 && x < 220) {
        if (y > 60 && y < 110) current_state.selected_char = CHAR_CAT;
        else if (y > 130 && y < 180) current_state.selected_char = CHAR_DOG;
        else if (y > 200 && y < 250) current_state.selected_char = CHAR_BIRD;
        Draw_Char_Page();
    }
}

static void Handle_Record_Touch(uint16_t x, uint16_t y)
{
    if (x > 170 && y > 270) { current_state.current_page = PAGE_MAIN; Draw_Main_Page(); return; }

    if (x > 60 && x < 180 && y > 140 && y < 260) {
        current_state.is_recording ^= 1;
        if (current_state.is_recording == 0) {
            current_state.active_overlay = OVERLAY_SAVE;
            Draw_Save_Overlay();
        } else {
            Draw_Record_Page();
        }
    }
}

static void Handle_File_Touch(uint16_t x, uint16_t y)
{
    if (x > 170 && y > 270) { current_state.current_page = PAGE_MAIN; Draw_Main_Page(); return; }

    for (uint8_t i = 0; i < current_state.file_count; i++)
    {
        uint16_t line_y = 60 + i * 35;
        if (y > line_y && y < line_y + 35 && x > 20 && x < 220)
        {
            strcpy(current_state.current_file, current_state.file_list[i]);
            current_state.active_overlay = OVERLAY_PLAYER;
            current_state.is_playing = 0;
            current_state.play_progress = 0;
            Draw_Player_Overlay();
            return;
        }
    }
}

// ==================================

void UI_Init(void)
{
    Draw_Main_Page();
}

void UI_Loop(void)
{
    strType_XPT2046_Coordinate coord;

    if (ucXPT2046_TouchFlag == 1)
    {
        ucXPT2046_TouchFlag = 0;
        if (XPT2046_Get_TouchedPoint(&coord, &strXPT2046_TouchPara))
        {
            uint16_t x = coord.x;
            uint16_t y = coord.y;

            if (current_state.active_overlay != OVERLAY_NONE)
            {
                if (current_state.active_overlay == OVERLAY_SAVE)
                {
                    if (x > 40 && x < 110 && y > 170 && y < 210) {
                        printf("Action: Save File to SD\n");
                        FIL testFile;
                        if(f_open(&testFile, "TEST.TXT", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
                            char buf[] = "Hello STM32!";
                            UINT bw;
                            f_write(&testFile, buf, sizeof(buf), &bw);
                            f_close(&testFile);
                            printf("Test File Created.\n");
                        }
                        current_state.active_overlay = OVERLAY_NONE;
                        current_state.current_page = PAGE_MAIN;
                        Draw_Main_Page();
                    }
                    else if (x > 130 && x < 200 && y > 170 && y < 210) {
                        current_state.active_overlay = OVERLAY_NONE;
                        Draw_Record_Page();
                    }
                }
                else if (current_state.active_overlay == OVERLAY_PLAYER)
                {
                    if (x > 160 && x < 220 && y > 180 && y < 220) {
                        current_state.active_overlay = OVERLAY_NONE;
                        current_state.is_playing = 0;
                        Draw_File_Page();
                    }
                    else if (!current_state.is_playing && x > 20 && x < 80 && y > 180 && y < 220) {
                        current_state.is_playing = 1;
                        Draw_Player_Overlay();
                        printf("Start Playing: %s\n", current_state.current_file);
                    }
                    else if (current_state.is_playing && x > 80 && x < 160 && y > 180 && y < 220) {
                        current_state.is_playing = 0;
                        Draw_Player_Overlay();
                        printf("Paused.\n");
                    }
                    else if (!current_state.is_playing && x > 90 && x < 150 && y > 180 && y < 220) {
                        char path[20];
                        sprintf(path, "%s", current_state.current_file);
                        if (f_unlink(path) == FR_OK) {
                            printf("Deleted: %s\n", path);
                            current_state.active_overlay = OVERLAY_NONE;
                            Draw_File_Page();
                        } else {
                            printf("Delete Failed!\n");
                        }
                    }
                }
                return;
            }

            switch (current_state.current_page)
            {
                case PAGE_MAIN: Handle_Main_Touch(x, y); break;
                case PAGE_CHAR: Handle_Char_Touch(x, y); break;
                case PAGE_RECORD: Handle_Record_Touch(x, y); break;
                case PAGE_FILE: Handle_File_Touch(x, y); break;
                default: break;
            }
        }
    }

    if (current_state.active_overlay == OVERLAY_PLAYER && current_state.is_playing)
    {
        current_state.play_progress += 2;
        if (current_state.play_progress > 100) current_state.play_progress = 0;

        uint16_t w = (200 * current_state.play_progress) / 100;
        LCD_Clear(20, 150, w, 12, BLUE);
        HAL_Delay(50);
    }
}
/*ToDo:
 * Debugging for VS1053 (Change the CLK Freq and shorter the wires)
 * Audio save into SD card
 * Audio read from SD card
 * Send audio and processing parameter to VS1053
 * Change page function of file list
 * Simulate FreeRTOS for concurrent tasks
 * Add an option of no voice effect in character select page
 * Optimize the button size and the area of valid region of touch handle
 * Add more coefficient to the calibration matrix
 * Cut the ear phone and soldering the lines to the amplifier
 * OverSampling of Button Detect
 * Set SDIO to 4-bit mode
 * Buy a new VS1053 module, speaker, ear phone, wires
*/
