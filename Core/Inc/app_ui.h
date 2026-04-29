#ifndef __APP_UI_H
#define __APP_UI_H

#include "stm32f1xx_hal.h"
#include "lcdtp.h"
#include "xpt2046.h"
#include "fatfs.h"


typedef enum {
    PAGE_MAIN = 0,
    PAGE_CHAR,
    PAGE_RECORD,
    PAGE_FILE,
} Page_ID;

typedef enum {
    OVERLAY_NONE = 0,
    OVERLAY_SAVE,
    OVERLAY_PLAYER
} Overlay_ID;

typedef enum {
    CHAR_NONE = 0,
    CHAR_CAT,
    CHAR_DOG,
    CHAR_BIRD
} Character_ID;

typedef struct {
    Page_ID current_page;
    Overlay_ID active_overlay;
    Character_ID selected_char;

    uint8_t is_recording;

    uint8_t is_playing;
    uint8_t play_progress;
    char current_file[13];

    char file_list[6][13];
    uint8_t file_count;
} App_State;

extern App_State current_state;

void UI_Init(void);
void UI_Loop(void);

#endif
