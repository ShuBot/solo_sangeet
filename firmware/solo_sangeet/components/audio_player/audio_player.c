#include "audio_player.h"
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"
#include "sd_card_fs.h"

static const char *TAG = "AUDIO";

RingbufHandle_t audio_rb;
static FILE *audio_fp = NULL;
static bool playing = false;
static TaskHandle_t reader_task_hdl = NULL;
EventGroupHandle_t audio_evt_grp = NULL;

void audio_player_init(void)
{
    // Setup SD Card and File System
    sd_fs_init();

    audio_evt_grp = xEventGroupCreate();
    configASSERT(audio_evt_grp);

    xTaskCreate(audio_control_task, "audio_ctrl", 4096, NULL, 6, NULL);

    // Create ring buffer
    if (audio_rb == NULL) {
        audio_rb = xRingbufferCreate(16 * 1024, RINGBUF_TYPE_BYTEBUF);
        assert(audio_rb);
    }

    playing = false;
}

bool audio_player_start(const char *path)
{
    if (playing) {
        ESP_LOGW(TAG, "Audio already playing");
        return false;
    }

    audio_fp = fopen(path, "rb");
    if (!audio_fp) {
        ESP_LOGE(TAG, "Failed to open WAV file");
        return false;
    }

    // Skip WAV header
    fseek(audio_fp, WAV_HEADER_SIZE, SEEK_SET);

    audio_rb = xRingbufferCreate(AUDIO_RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    if (!audio_rb) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        fclose(audio_fp);
        return false;
    }

    playing = true;
    xEventGroupSetBits(audio_evt_grp, EVT_AUDIO_PLAYING);

    xTaskCreate(
        audio_reader_task,
        "audio_reader",
        4096,
        NULL,
        5,
        &reader_task_hdl
    );

    ESP_LOGI(TAG, "Audio playback started");
    return true;
}

void audio_player_stop(void)
{
    playing = false;
    xEventGroupClearBits(audio_evt_grp, EVT_AUDIO_PLAYING);

    if (audio_rb) {
        vRingbufferDelete(audio_rb);
        audio_rb = NULL;
    }

    if (audio_fp) {
        fclose(audio_fp);
        audio_fp = NULL;
    }

    ESP_LOGI(TAG, "Audio playback stopped");
}

void audio_reader_task(void *arg)
{
    uint8_t buffer[AUDIO_READ_CHUNK];

    ESP_LOGI(TAG, "Audio reader task started");

    while (playing) {
        size_t rd = fread(buffer, 1, sizeof(buffer), audio_fp);

        if (rd == 0) {
            ESP_LOGI(TAG, "End of WAV file");
            break;
        }

        BaseType_t ok = xRingbufferSend(audio_rb, buffer, rd, pdMS_TO_TICKS(100));

        if (ok != pdTRUE) {
            ESP_LOGW(TAG, "Ring buffer full");
        }
    }

    playing = false;

    if (audio_fp) {
        fclose(audio_fp);
        audio_fp = NULL;
    }

    ESP_LOGI(TAG, "Audio reader task exiting");
    vTaskDelete(NULL);
}


bool audio_player_is_playing(void)
{
    return playing;
}

void audio_control_task(void *arg)
{
    ESP_LOGI(TAG, "Audio control task started");
    while (1) {
        EventBits_t bits = xEventGroupWaitBits(
            audio_evt_grp,
            EVT_A2DP_STREAMING | EVT_USER_PLAY,
            pdFALSE,
            pdTRUE,      // wait for BOTH
            portMAX_DELAY
        );

        if ((bits & (EVT_A2DP_STREAMING | EVT_USER_PLAY)) ==
            (EVT_A2DP_STREAMING | EVT_USER_PLAY)) {

            if (!audio_player_is_playing() && (bits & EVT_AUDIO_PLAYING) == 0) {
                ESP_LOGI("AUDIO", "Starting playback");
                audio_player_start("/sdcard/TEST_00.WAV");
                xEventGroupSetBits(audio_evt_grp, EVT_AUDIO_PLAYING);
            }
        }
    }
}

lv_style_t style_bg;
lv_style_t style_title;
lv_style_t style_play;

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


static lv_obj_t *label_title;
static lv_obj_t *bar_progress;
static lv_obj_t *btn_play;
static lv_obj_t *icon_play;
static lv_obj_t *btn_next;
static lv_obj_t *btn_prev;

static bool is_playing = false;

static void ui_play_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Audio playback started");
    xEventGroupSetBits(audio_evt_grp, EVT_USER_PLAY);
}

static void ui_pause_cb(lv_event_t *e)
{
    ESP_LOGI(TAG, "Audio playback paused");
    xEventGroupClearBits(audio_evt_grp, EVT_USER_PLAY);
    xEventGroupClearBits(audio_evt_grp, EVT_AUDIO_PLAYING);
    audio_player_stop();
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

void audio_player_ui_init(void)
{ 
    lv_obj_t *scr = lv_obj_create(NULL);
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