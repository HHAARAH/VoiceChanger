#ifndef __APP_UI_H
#define __APP_UI_H

#include "stm32f1xx_hal.h"
#include "lcdtp.h"
#include "xpt2046.h"
#include "fatfs.h"

#define FILES_PER_PAGE 5

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
    CHAR_NORMAL,
    CHAR_CAT,
    CHAR_DOG,
    CHAR_BIRD
} Character_ID;

typedef struct {
    /* Pages & overlays */
    Page_ID     current_page;
    Overlay_ID  active_overlay;
    Character_ID selected_char;

    /* Recording */
    uint8_t     is_recording;
    FIL         rec_fil;
    uint32_t    rec_sample_rate;
    uint32_t    rec_data_bytes;
    uint8_t     rec_buffer[8192];
    uint16_t    rec_buf_len;

    /* Playback */
    uint8_t     is_playing;
    uint8_t     play_progress;
    char        current_file[13];
    FIL         play_fil;
    uint32_t    play_file_size;
    uint32_t    play_bytes_read;
    uint8_t     play_buffer[512];
    uint16_t    play_buf_idx;
    uint16_t    play_buf_len;

    /* File browser */
    char        file_list[FILES_PER_PAGE][13];
    uint8_t     file_count;
    uint8_t     file_page;
    uint16_t    total_files;
} App_State;

extern App_State current_state;

void UI_Init(void);
void UI_Loop(void);

#endif
