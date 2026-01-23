#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"
#include "lvgl.h"

extern EventGroupHandle_t audio_evt_grp;

#define EVT_A2DP_STREAMING   (1 << 0)
#define EVT_USER_PLAY       (1 << 1)
#define EVT_AUDIO_PLAYING   (1 << 2)

#define AUDIO_RINGBUF_SIZE   (32 * 1024)   // or even 64 KB
#define AUDIO_READ_CHUNK    (2048)         // or 4096
#define WAV_HEADER_SIZE    44

extern RingbufHandle_t audio_rb;
extern QueueHandle_t audio_cmd_q;
extern const char *current_file;

// Audio Player Commands
typedef enum {
    AUDIO_CMD_NONE = 0,
    AUDIO_CMD_PLAY,
    AUDIO_CMD_PAUSE,
    AUDIO_CMD_STOP,
    AUDIO_CMD_EOF,
    AUDIO_CMD_BT_CONNECTED,
    AUDIO_CMD_BT_DISCONNECTED,
} audio_cmd_t;

// Audio Player States
typedef enum {
    AUDIO_STATE_IDLE,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_PAUSED,
    AUDIO_STATE_STOPPED,
} audio_state_t;

void audio_player_init(void);
bool audio_player_start(const char *path);
void audio_player_stop(void);
bool audio_player_is_playing(void);

// int32_t audio_player_read(uint8_t *data, int32_t len);
void audio_reader_task(void *arg);
void audio_control_task(void *arg);

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

#endif // AUDIO_PLAYER_H