#include "audio_player.h"
#include "ili9341_driver.h"
#include "bt_audio.h"
#include "ss_ui_theme.h"

static const char *TAG = "AUDIO_UI";

/* ------------------ Globals ------------------ */
static lv_obj_t * menu;
static lv_obj_t * menu_scr;
static lv_obj_t * top_bar;
static lv_obj_t * label_battery;
static lv_obj_t * label_wifi;
static lv_obj_t * label_bt;

/* Pages */
static lv_obj_t * page_home;
static lv_obj_t * page_options;
static lv_obj_t * page_bt;
static lv_obj_t * page_wifi;
static lv_obj_t * music_scr;

// BT List
lv_obj_t * bt_list;
/* ------------------ Audio Player UI ------------------ */
// UI Styles
lv_style_t style_bg;
lv_style_t style_title;
lv_style_t style_play;

// UI elements
static lv_obj_t *label_title;
static lv_obj_t *bar_progress;
static lv_obj_t *btn_play;
static lv_obj_t *icon_play;
static lv_obj_t *btn_next;
static lv_obj_t *btn_prev;

static bool is_playing = false;

// Global function declarations
void audio_player_page_create(lv_obj_t * scr);
void create_bottom_nav(lv_obj_t * parent);
void create_top_status_bar(lv_obj_t * parent);

void ui_player_style_init(void)
{
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, lv_color_black());
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_white());
    lv_style_set_text_font(&style_title, &lv_font_montserrat_14);
    
    lv_style_init(&style_play);
    lv_style_set_bg_color(&style_play, lv_color_hex(0xFF4081));
    lv_style_set_bg_opa(&style_play, LV_OPA_COVER);
}

static void ui_cont_apply_theme(lv_obj_t * cont)
{
    lv_obj_add_style(cont, &style_cont_item, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    // Border
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cont, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cont, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(cont, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void ui_cont_label_apply_theme(lv_obj_t * label)
{
    lv_obj_add_style(label, &style_cont_text, LV_PART_MAIN);
    lv_obj_set_width(label, LV_PCT(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
}

static void ui_list_apply_theme(lv_obj_t * list)
{
    lv_obj_add_style(list, &style_list_bg, LV_PART_MAIN);
}

static void ui_list_item_apply_theme(lv_obj_t * btn)
{
    lv_obj_add_style(btn, &style_list_item, LV_PART_MAIN);
    lv_obj_add_style(lv_obj_get_child(btn, 1), &style_list_text, LV_PART_MAIN);
}

static void menu_item_vertical(lv_obj_t * cont)
{
    /* Make container vertical */
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);

    /* Center content */
    lv_obj_set_flex_align(
        cont,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    /* Touch-friendly size */
    lv_obj_set_height(cont, 90);
    lv_obj_set_style_pad_ver(cont, 12, 0);
    lv_obj_set_style_pad_hor(cont, 16, 0);
}

static void ui_play_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Audio playback started");
    audio_cmd_t cmd = AUDIO_CMD_PLAY;
    xQueueSend(audio_cmd_q, &cmd, 0);

}

static void ui_pause_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Audio playback paused");
    audio_cmd_t cmd = AUDIO_CMD_PAUSE;
    xQueueSend(audio_cmd_q, &cmd, 0);

}

void ui_reset_play_button(void)
{
    is_playing = !is_playing;

    // Uncheck toggle button
    lv_obj_clear_state(btn_play, LV_STATE_CHECKED);

    // Optional: change icon back to PLAY
    lv_label_set_text(icon_play, LV_SYMBOL_PLAY);

    ESP_LOGI(TAG, "Play button reset after EOF");
}

static void player_btn_event_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    if(btn == btn_play) {
        is_playing = !is_playing;

        lv_label_set_text(
            icon_play,
            is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY
        );

        if(is_playing) {
            ui_play_cb(e);
        } else {
            ui_pause_cb(e);
        }
        /* later: call player_play() / player_pause() */
    }
    else if(btn == btn_next) {
        /* later: player_next() */
    }
    else if(btn == btn_prev) {
        /* later: player_prev() */
    }
}

/* ------------------ Event callbacks ------------------ */
// Music player UI
void audio_player_page_create(lv_obj_t * scr)
{
    ui_player_style_init();
    // lv_obj_t *scr = lv_scr_act();
    lv_obj_add_style(scr, &style_bg, 0);
    lv_scr_load(scr);

    /* Track title */
    label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Solo Sangeet");
    lv_obj_add_style(label_title, &style_title, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 12);

    /* Progress bar */
    bar_progress = lv_bar_create(scr);
    lv_obj_set_size(bar_progress, 200, 6);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_obj_align(bar_progress, LV_ALIGN_CENTER, 0, 0);

    /* --- PREV button --- */
    btn_prev = lv_btn_create(scr);
    lv_obj_set_size(btn_prev, 48, 48);
    lv_obj_align(btn_prev, LV_ALIGN_CENTER, -80, 50);
    lv_obj_add_event_cb(btn_prev, player_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon_prev = lv_label_create(btn_prev);
    lv_label_set_text(icon_prev, LV_SYMBOL_PREV);
    lv_obj_center(icon_prev);

    /* --- PLAY button (circular) --- */
    btn_play = lv_btn_create(scr);
    lv_obj_set_size(btn_play, 64, 64);
    lv_obj_add_style(btn_play, &style_play, 0);
    lv_obj_align(btn_play, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_radius(btn_play, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(btn_play, player_btn_event_cb, LV_EVENT_CLICKED, NULL);

    icon_play = lv_label_create(btn_play);
    lv_label_set_text(icon_play, LV_SYMBOL_PLAY);
    lv_obj_center(icon_play);

    /* --- NEXT button --- */
    btn_next = lv_btn_create(scr);
    lv_obj_set_size(btn_next, 48, 48);
    lv_obj_align(btn_next, LV_ALIGN_CENTER, 80, 50);
    lv_obj_add_event_cb(btn_next, player_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon_next = lv_label_create(btn_next);
    lv_label_set_text(icon_next, LV_SYMBOL_NEXT);
    lv_obj_center(icon_next);

    // Nav Bar for music screen
    create_bottom_nav(music_scr);
    // Add top status bar to music screen
    create_top_status_bar(music_scr);
    
    // TODO: Remove later, for testing only.
    ui_set_battery_level(73);
    // ui_set_wifi(true);
    // ui_set_bt(false);
    log_mem(TAG);
}

static void music_player_create(void)
{
    music_scr = lv_obj_create(NULL);   // NEW SCREEN
    lv_obj_clear_flag(music_scr, LV_OBJ_FLAG_SCROLLABLE);

    // Create audio player UI on this screen
    audio_player_page_create(music_scr);
}

/* Music player will be a full screen later */
static void music_open_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(!music_scr) { music_player_create(); }
    lv_scr_load_anim(music_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

static void brightness_cb(lv_event_t * e)
{
    int val = lv_slider_get_value(lv_event_get_target(e));
    ESP_LOGI(TAG, "Brightness: %d", val);
    ili9341_set_brightness(val);
}

static void volume_cb(lv_event_t * e)
{
    int val = lv_slider_get_value(lv_event_get_target(e));
    ESP_LOGI(TAG, "Volume: %d", val);
    // audio_set_volume(val);
}

// Top status bar UI Callbacks
void ui_set_battery_level(uint8_t percent)
{
    if(percent > 80) {
        lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_FULL);
    }
    else if(percent > 60) {
        lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_3);
    }
    else if(percent > 40) {
        lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_2);
    }
    else if(percent > 20) {
        lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_1);
    }
    else {
        lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_EMPTY);
    }
}

void ui_set_wifi(bool connected)
{
    lv_label_set_text(label_wifi,
        connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
}

void ui_set_bt(bool connected)
{
    lv_label_set_text(label_bt,
        connected ? LV_SYMBOL_BLUETOOTH : LV_SYMBOL_CLOSE);
}

/* ------------------ Page creators ------------------ */
static void menu_item_make_touch_friendly(lv_obj_t * cont)
{
    lv_obj_set_style_pad_all(cont, 10, LV_PART_MAIN);
    lv_obj_set_height(cont, 60);
}

static lv_obj_t * create_home_page(lv_obj_t * menu)
{
    lv_obj_t * page = lv_menu_page_create(menu, "Home");
    lv_obj_t * section = lv_menu_section_create(page);

    /* Bluetooth */
    lv_obj_t * cont_bt = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont_bt);
    menu_item_make_touch_friendly(cont_bt);
    lv_obj_t * bt_label = lv_label_create(cont_bt);
    lv_label_set_text(bt_label, "Bluetooth  " LV_SYMBOL_BLUETOOTH);
    ui_cont_label_apply_theme(bt_label);
    lv_menu_set_load_page_event(menu, cont_bt, page_bt);

    /* Music */
    lv_obj_t * cont_music = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont_music);
    menu_item_make_touch_friendly(cont_music);
    lv_obj_t * music_label = lv_label_create(cont_music);
    lv_label_set_text(music_label, "Music Player");
    ui_cont_label_apply_theme(music_label);
    lv_obj_add_flag(cont_music, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont_music, music_open_cb, LV_EVENT_PRESSED, NULL);

    /* WiFi */
    lv_obj_t * cont_wifi = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont_wifi);
    menu_item_make_touch_friendly(cont_wifi);
    lv_obj_t * wifi_label = lv_label_create(cont_wifi);
    lv_label_set_text(wifi_label, "WiFi     " LV_SYMBOL_WIFI);
    ui_cont_label_apply_theme(wifi_label);
    lv_menu_set_load_page_event(menu, cont_wifi, page_wifi);

    return page;
}

static lv_obj_t * create_options_page(lv_obj_t * menu)
{
    lv_obj_t * page = lv_menu_page_create(menu, "Options");
    lv_obj_t * section = lv_menu_section_create(page);

    /* ---------- Brightness ---------- */
    lv_obj_t * cont_bright = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont_bright);
    menu_item_vertical(cont_bright);

    lv_obj_t * lbl_bright = lv_label_create(cont_bright);
    lv_label_set_text(lbl_bright, "Display Brightness");
    lv_obj_set_style_text_align(lbl_bright, LV_TEXT_ALIGN_CENTER, 0);
    ui_cont_label_apply_theme(lbl_bright);

    lv_obj_t * slider_bright = lv_slider_create(cont_bright);
    lv_slider_set_range(slider_bright, 1, 100);
    lv_slider_set_value(slider_bright, init_brightness, LV_ANIM_OFF);
    lv_obj_set_width(slider_bright, LV_PCT(80));
    // Setup Callback
    lv_obj_add_event_cb(slider_bright, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ---------- Volume ---------- */
    lv_obj_t * cont_vol = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont_vol);
    menu_item_vertical(cont_vol);

    lv_obj_t * lbl_vol = lv_label_create(cont_vol);
    lv_label_set_text(lbl_vol, "System Volume");
    lv_obj_set_style_text_align(lbl_vol, LV_TEXT_ALIGN_CENTER, 0);
    ui_cont_label_apply_theme(lbl_vol);

    lv_obj_t * slider_vol = lv_slider_create(cont_vol);
    lv_slider_set_range(slider_vol, 0, 100);
    lv_slider_set_value(slider_vol, 50, LV_ANIM_OFF);
    lv_obj_set_width(slider_vol, LV_PCT(80));
    // Setup Callback
    lv_obj_add_event_cb(slider_vol, volume_cb, LV_EVENT_VALUE_CHANGED, NULL);

    return page;
}

static void device_selected_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int index = (int)lv_obj_get_user_data(btn);

    ESP_LOGI(TAG, "BT Button Pressed, index: %d", index);
    bt_user_select_device(index);
}

void ui_bt_devices_updated(void)
{
    for (int i = 0; i < s_bt_scan_count; i++) {
        
        ESP_LOGI(TAG, "Updating BT List with: %s", s_bt_scan_list[i].name);

        lv_obj_t *btn = lv_list_add_button(
            bt_list,
            LV_SYMBOL_AUDIO,
            s_bt_scan_list[i].name);
            
        lv_obj_set_user_data(btn, (void *)i);
        lv_obj_add_event_cb(btn, device_selected_cb, LV_EVENT_CLICKED, NULL);
        ui_list_item_apply_theme(btn);
    }
}

static lv_obj_t * create_bt_page(lv_obj_t * menu)
{
    lv_obj_t * page = lv_menu_page_create(menu, "Bluetooth");
    lv_obj_t * section = lv_menu_section_create(page);

    lv_obj_t * cont = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont);
    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, "Scanning Devices:");
    ui_cont_label_apply_theme(label);

    /* Dummy items (replace later with scan results) */
    bt_list = lv_list_create(page);
    lv_obj_set_size(bt_list, LV_PCT(100), LV_PCT(70));
    lv_obj_align(bt_list, LV_ALIGN_BOTTOM_MID, 0, -5);
    ui_list_apply_theme(bt_list);

    return page;
}

static lv_obj_t * create_wifi_page(lv_obj_t * menu)
{
    lv_obj_t * page = lv_menu_page_create(menu, "WiFi");
    lv_obj_t * section = lv_menu_section_create(page);

    lv_obj_t * cont = lv_menu_cont_create(section);
    ui_cont_apply_theme(cont);
    
    lv_obj_t * label = lv_label_create(cont);
    lv_label_set_text(label, "Scan Networks");
    ui_cont_label_apply_theme(label);    

    lv_obj_t * list = lv_list_create(page);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(70));
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
    ui_list_apply_theme(list);

    lv_obj_t * btn;
    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "Home_WiFi");
    ui_list_item_apply_theme(btn);

    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "Office_AP");
    ui_list_item_apply_theme(btn);

    return page;
}


static void nav_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    /* If music screen is active → go back to menu */
    if(lv_scr_act() == music_scr) {
        lv_scr_load_anim(menu_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        return;
    }

    /* If inside menu → go to home page */
    lv_menu_set_page(menu, page_home);
}

static void nav_menu_cb(lv_event_t * e)
{
    LV_UNUSED(e);

    if(lv_scr_act() != menu_scr) {
        lv_scr_load_anim(menu_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

    lv_menu_set_page(menu, page_home);
}

static void nav_options_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    
    /* Make sure we're on menu screen */
    if(lv_scr_act() != menu_scr) {
        lv_scr_load_anim(menu_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

    lv_menu_set_page(menu, page_options);
}

void create_bottom_nav(lv_obj_t * parent)
{
    int btn_height = 30;
    int btn_length = 80;

    /* OPTIONS */
    lv_obj_t * btn_opt = lv_btn_create(parent);
    lv_obj_set_size(btn_opt, btn_length, btn_height);
    lv_obj_set_align(btn_opt, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_event_cb(btn_opt, nav_options_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * btn_opt_label = lv_label_create(btn_opt);
    lv_label_set_text(btn_opt_label, " " LV_SYMBOL_SETTINGS " ");
    lv_obj_align_to(btn_opt_label, btn_opt, LV_ALIGN_CENTER, 0, 0);

    /* MENU (Home) */
    lv_obj_t * btn_menu = lv_btn_create(parent);
    lv_obj_set_size(btn_menu, btn_length, btn_height);
    lv_obj_set_align(btn_menu, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_event_cb(btn_menu, nav_menu_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * btn_menu_label = lv_label_create(btn_menu);
    lv_label_set_text(btn_menu_label, " " LV_SYMBOL_HOME " ");
    lv_obj_align_to(btn_menu_label, btn_menu, LV_ALIGN_CENTER, 0, 0);

    /* BACK */
    lv_obj_t * btn_back = lv_btn_create(parent);
    lv_obj_set_size(btn_back, btn_length, btn_height);
    lv_obj_set_align(btn_back, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_add_event_cb(btn_back, nav_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, "  " LV_SYMBOL_LEFT " ");
    lv_obj_align_to(btn_back_label, btn_back, LV_ALIGN_CENTER, 0, 0);
}

void create_top_status_bar(lv_obj_t * parent)
{
    top_bar = lv_obj_create(parent);
    lv_obj_set_size(top_bar, LV_PCT(100), 36);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(top_bar, lv_color_black(), 0);
    lv_obj_set_style_pad_hor(top_bar, 10, 0);
    lv_obj_set_style_pad_ver(top_bar, 4, 0);

    /* Flex layout: left → right */
    lv_obj_set_flex_flow(top_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        top_bar,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER
    );

    // LEFT container to occupy space
    lv_obj_t * left = lv_obj_create(top_bar);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);

    label_wifi = lv_label_create(top_bar);
    lv_label_set_text(label_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(label_wifi, lv_color_white(), 0);

    label_bt = lv_label_create(top_bar);
    lv_label_set_text(label_bt, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(label_bt, lv_color_white(), 0);

    /* RIGHT: Battery */
    label_battery = lv_label_create(top_bar);
    lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_set_style_text_color(label_battery, lv_color_white(), 0);
}

void audio_player_ui_init(lv_disp_t *disp)
{
    // Init ui theme
    ui_theme_init();

    // Create menu
    menu_scr = lv_display_get_screen_active(disp);
    menu = lv_menu_create(menu_scr);
    lv_obj_set_size(menu, LV_PCT(100), LV_PCT(100));
    lv_obj_center(menu);
    lv_obj_set_style_pad_top(menu, 36, 0);   // top bar height
    lv_obj_set_style_pad_bottom(menu, 60, 0); // bottom bar height

    // Apply menu background theme
    lv_obj_add_style(menu, &style_menu_bg, 0);

    // Hide lv_menu main header
    lv_obj_set_height(lv_menu_get_main_header(menu), 0);

    // Create pages
    page_bt      = create_bt_page(menu);
    page_wifi    = create_wifi_page(menu);
    page_home    = create_home_page(menu);
    page_options = create_options_page(menu);

    // Set root page
    lv_menu_set_page(menu, page_home);

    // Nav bar + Status bar
    create_bottom_nav(menu_scr);
    create_top_status_bar(menu_scr);
}