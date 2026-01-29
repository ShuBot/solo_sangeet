#include "audio_player.h"
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"
#include "sd_card_fs.h"

RingbufHandle_t audio_rb;
TaskHandle_t reader_task_hdl = NULL;
const char *current_file = "/sdcard/TEST_00.WAV";
QueueHandle_t audio_cmd_q;

static const char *TAG = "AUDIO";
static FILE *audio_fp = NULL;
static bool playing = false;
static volatile bool stop_requested = false;

void log_mem(const char *tag)
{
    ESP_LOGI(tag,
        "Free heap: %u | Internal: %u | Largest block: %u",
        esp_get_free_heap_size(),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)
    );
}

void audio_player_init(void)
{
    // Setup SD Card and File System
    sd_fs_init();

    audio_cmd_q = xQueueCreate(8, sizeof(audio_cmd_t));
    configASSERT(audio_cmd_q);

    // Create ring buffer ONCE
    audio_rb = xRingbufferCreate(AUDIO_RINGBUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    configASSERT(audio_rb);

    // Start audio control task
    xTaskCreate(audio_control_task, "audio_ctrl", 4096, NULL, 6, NULL);

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

    fseek(audio_fp, WAV_HEADER_SIZE, SEEK_SET);

    xRingbufferPrintInfo(audio_rb);
    playing = true;
    
    xTaskCreate(audio_reader_task, "audio_reader", 4096 * 2, NULL, 5, &reader_task_hdl);

    ESP_LOGI(TAG, "Audio playback started");
    return true;
}

void audio_player_stop(void)
{
    if (!playing) {
        return;
    }

    stop_requested = true;
    ESP_LOGI(TAG, "Audio playback stopped");
}

void audio_reader_task(void *arg)
{
    uint8_t buffer[AUDIO_READ_CHUNK];

    ESP_LOGI(TAG, "Audio reader task started");

    while (!stop_requested) {

        size_t bytes = fread(buffer, 1, sizeof(buffer), audio_fp);
        if (bytes == 0) {
            ESP_LOGI(TAG, "End of WAV file");
     
            audio_cmd_t cmd = AUDIO_CMD_EOF;
            xQueueSend(audio_cmd_q, &cmd, 0);

            break;
        }

        if (xRingbufferSend(audio_rb, buffer, bytes,
                            pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "Ring buffer full");
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    /* ---- CLEANUP SECTION ---- */

    ESP_LOGI(TAG, "Audio reader task exiting");

    stop_requested = false;
    playing = false;

    if (audio_fp) {
        fclose(audio_fp);
        audio_fp = NULL;
    }

    vTaskDelete(NULL);
}


bool audio_player_is_playing(void)
{
    return playing;
}

void audio_control_task(void *arg)
{
    audio_state_t state = AUDIO_STATE_IDLE;
    audio_cmd_t cmd = AUDIO_CMD_NONE;

    while (1) {
        if (xQueueReceive(audio_cmd_q, &cmd, portMAX_DELAY)) {

            ESP_LOGI(TAG, "Audio CMD %d in state %d", cmd, state);

            switch (state) {

            case AUDIO_STATE_IDLE:
            case AUDIO_STATE_STOPPED:
                if (cmd == AUDIO_CMD_PLAY) {
                    audio_player_start(current_file);
                    state = AUDIO_STATE_PLAYING;
                }
                break;

            case AUDIO_STATE_PLAYING:
                {
                    log_mem(TAG);
                    switch(cmd) {
                        case AUDIO_CMD_PAUSE:
                            audio_player_stop();
                            state = AUDIO_STATE_PAUSED;
                            break;
                        // case AUDIO_CMD_STOP:
                        //     audio_player_stop();
                        //     state = AUDIO_STATE_STOPPED;
                        //     break;
                        case AUDIO_CMD_EOF:
                            audio_player_stop();
                            state = AUDIO_STATE_STOPPED;
                            // ui_notify_play_reset();  // async LVGL
                            ui_reset_play_button();
                            break;
                        default:
                            break;
                    }
                }
                break;

            case AUDIO_STATE_PAUSED:
                if (cmd == AUDIO_CMD_PLAY) {
                    audio_player_start(current_file);
                    state = AUDIO_STATE_PLAYING;
                } else if (cmd == AUDIO_CMD_STOP) {
                    state = AUDIO_STATE_STOPPED;
                }
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // keep WDT happy
    }
}
