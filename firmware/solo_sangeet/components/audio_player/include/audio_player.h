#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"

extern EventGroupHandle_t audio_evt_grp;

#define EVT_A2DP_STREAMING   (1 << 0)
#define EVT_USER_PLAY       (1 << 1)
#define EVT_AUDIO_PLAYING   (1 << 2)

#define AUDIO_RINGBUF_SIZE   (32 * 1024)   // or even 64 KB
#define AUDIO_READ_CHUNK    (2048)         // or 4096
#define WAV_HEADER_SIZE    44

extern RingbufHandle_t audio_rb;

void audio_player_init(void);
bool audio_player_start(const char *path);
void audio_player_stop(void);
bool audio_player_is_playing(void);

// int32_t audio_player_read(uint8_t *data, int32_t len);
void audio_reader_task(void *arg);
void audio_control_task(void *arg);

void audio_player_ui_init(void);

#endif // AUDIO_PLAYER_H