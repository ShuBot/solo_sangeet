#include "ui_theme.h"

ui_theme_t ui_theme;   // uninitialized global

lv_style_t style_menu_bg;
lv_style_t style_menu_item;
lv_style_t style_menu_text;

lv_style_t style_cont_bg;
lv_style_t style_cont_item;
lv_style_t style_cont_text;

lv_style_t style_list_bg;
lv_style_t style_list_item;
lv_style_t style_list_text;

void ui_theme_init(void)
{
    /* Assign values at runtime */
    ui_theme.fg   = lv_color_white();
    ui_theme.bg   = lv_color_black();
    ui_theme.font = &lv_font_montserrat_14;

    /* Init Menu styles */
    lv_style_init(&style_menu_bg);
    lv_style_set_bg_color(&style_menu_bg, ui_theme.bg);
    lv_style_set_bg_opa(&style_menu_bg, LV_OPA_COVER);

    lv_style_init(&style_menu_item);
    lv_style_set_bg_color(&style_menu_item, ui_theme.bg);
    lv_style_set_pad_ver(&style_menu_item, 14);
    lv_style_set_pad_hor(&style_menu_item, 16);
    lv_style_set_height(&style_menu_item, 60);

    lv_style_init(&style_menu_text);
    lv_style_set_text_color(&style_menu_text, ui_theme.fg);
    lv_style_set_text_font(&style_menu_text, ui_theme.font);

    /* Init Container styles */
    lv_style_init(&style_cont_bg);
    lv_style_set_bg_color(&style_cont_bg, ui_theme.bg);
    lv_style_set_bg_opa(&style_cont_bg, LV_OPA_COVER);

    lv_style_init(&style_cont_item);
    lv_style_set_bg_color(&style_cont_item, ui_theme.bg);

    lv_style_init(&style_cont_text);
    lv_style_set_text_color(&style_cont_text, ui_theme.fg);
    lv_style_set_text_font(&style_cont_text, ui_theme.font);

    /* Init List styles */

    /* ---------- List background ---------- */
    lv_style_init(&style_list_bg);
    lv_style_set_bg_color(&style_list_bg, ui_theme.bg);
    lv_style_set_bg_opa(&style_list_bg, LV_OPA_COVER);
    lv_style_set_pad_all(&style_list_bg, 4);

    /* ---------- List item (row) ---------- */
    lv_style_init(&style_list_item);
    lv_style_set_bg_color(&style_list_item, ui_theme.bg);
    lv_style_set_pad_ver(&style_list_item, 5);
    lv_style_set_pad_hor(&style_list_item, 5);
    lv_style_set_height(&style_list_item, 30);

    /* ---------- List item text ---------- */
    lv_style_init(&style_list_text);
    lv_style_set_text_color(&style_list_text, ui_theme.fg);
    lv_style_set_text_font(&style_list_text, ui_theme.font);

}
