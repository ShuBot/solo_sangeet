#include "audio_player.h"

static const char *TAG = "AUDIO_UI";

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

void ui_player_style_init(void)
{
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, lv_color_hex(0x1E1E1E));
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_white());
    lv_style_set_text_font(&style_title, &lv_font_montserrat_14);
    
    lv_style_init(&style_play);
    lv_style_set_bg_color(&style_play, lv_color_hex(0xFF4081));
    lv_style_set_bg_opa(&style_play, LV_OPA_COVER);
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

void audio_player_ui_init(lv_disp_t *disp)
{
    ui_player_style_init();
    
    lv_obj_t *scr = lv_display_get_screen_active(disp);
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
    lv_obj_align(bar_progress, LV_ALIGN_CENTER, 0, 20);

    /* --- PREV button --- */
    btn_prev = lv_btn_create(scr);
    lv_obj_set_size(btn_prev, 48, 48);
    lv_obj_align(btn_prev, LV_ALIGN_CENTER, -80, 70);
    lv_obj_add_event_cb(btn_prev, player_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon_prev = lv_label_create(btn_prev);
    lv_label_set_text(icon_prev, LV_SYMBOL_PREV);
    lv_obj_center(icon_prev);

    /* --- PLAY button (circular) --- */
    btn_play = lv_btn_create(scr);
    lv_obj_set_size(btn_play, 64, 64);
    lv_obj_add_style(btn_play, &style_play, 0);
    lv_obj_align(btn_play, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_style_radius(btn_play, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(btn_play, player_btn_event_cb, LV_EVENT_CLICKED, NULL);

    icon_play = lv_label_create(btn_play);
    lv_label_set_text(icon_play, LV_SYMBOL_PLAY);
    lv_obj_center(icon_play);

    /* --- NEXT button --- */
    btn_next = lv_btn_create(scr);
    lv_obj_set_size(btn_next, 48, 48);
    lv_obj_align(btn_next, LV_ALIGN_CENTER, 80, 70);
    lv_obj_add_event_cb(btn_next, player_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon_next = lv_label_create(btn_next);
    lv_label_set_text(icon_next, LV_SYMBOL_NEXT);
    lv_obj_center(icon_next);

}