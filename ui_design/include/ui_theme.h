#pragma once
#include "lvgl.h"

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
