#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#pragma once
#include "lvgl.h"

/*
* Marcos for color definitions in RGB565 format
* These can be used to fill the screen with specific colors
*/
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_GREY          0xbdf7
#define COLOR_LIGHT_GREY    0xe73c
#define COLOR_DARK_GREY     0x8410
#define COLOR_ORANGE        0xFD20
#define COLOR_BROWN         0xBC40
#define COLOR_PURPLE        0x780F
#define COLOR_NAVY          0x0010
#define COLOR_MAROON        0x8000
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_PINK          0xF818


typedef struct {
    lv_color_t bg;
    lv_color_t fg;
    const lv_font_t * font;
} ui_theme_t;

extern ui_theme_t ui_theme;

/* ---------- LVGL styles ---------- */
extern lv_style_t style_menu_bg;
extern lv_style_t style_menu_item;
extern lv_style_t style_menu_text;

extern lv_style_t style_cont_bg;
extern lv_style_t style_cont_item;
extern lv_style_t style_cont_text;

extern lv_style_t style_list_bg;
extern lv_style_t style_list_item;
extern lv_style_t style_list_text;

/* Call once during init */
void ui_theme_init(void);


void audio_player_ui_init(lv_disp_t *disp);

typedef void (*audio_player_event_cb_t)(void);
void audio_player_register_eof_cb(audio_player_event_cb_t cb);
void ui_audio_eof_cb(void);
void ui_reset_play_button(void);
// BT UI
void ui_bt_devices_updated(void);
void ui_set_bt(bool connected);
// WiFi UI
void ui_set_wifi(bool connected);
// Battery UI
void ui_set_battery_level(uint8_t percent);

#endif // UI_MANAGER_H