#ifndef __BT_AUDIO_H__
#define __BT_AUDIO_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

// BT Discovery
typedef enum {
    BT_APP_EVT_STACK_UP = 0,
    BT_APP_EVT_SCAN_LIST_UPDATED,
    BT_APP_EVT_CONNECT_DEVICE,
} bt_app_evt_t;

#define MAX_BT_DEVICES 10
#define BT_NAME_LEN    32

typedef struct {
    esp_bd_addr_t bda;
    char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
    int rssi;
    bool in_use;
} bt_scan_device_t;

extern bt_scan_device_t s_bt_scan_list[MAX_BT_DEVICES];
extern int s_bt_scan_count;

// BT Discovery

/* log tag */
#define BT_APP_CORE_TAG             "BT_APP_CORE"

/* signal for dispatcher */
#define BT_APP_SIG_WORK_DISPATCH    (0x01)

/**
 * @brief    handler for the dispatched work
 *
 * @param [in] event  message event id
 * @param [in] param  pointer to the parameter
 */
typedef void (* bt_app_cb_t) (uint16_t event, void *param);

/* message to be sent */
typedef struct {
    uint16_t             sig;      /*!< signal to bt_app_task */
    uint16_t             event;    /*!< message event id */
    bt_app_cb_t          cb;       /*!< context switch callback */
    void                 *param;   /*!< parameter area needs to be last */
} bt_app_msg_t;

/**
 * @brief    parameter deep-copy function to be customized
 *
 * @param [in] p_dest  pointer to the destination
 * @param [in] p_src   pointer to the source
 * @param [in] len     data length in byte
 */
typedef void (* bt_app_copy_cb_t) (void *p_dest, void *p_src, int len);

/**
 * @brief    work dispatcher for the application task
 *
 * @param [in] p_cback       handler for the dispatched work (event handler)
 * @param [in] event         message event id
 * @param [in] p_params      pointer to the parameter
 * @param [in] param_len     length of the parameter
 * @param [in] p_copy_cback  parameter deep-copy function
 *
 * @return  true if work dispatch successfully, false otherwise
 */
bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);

/**
 * @brief    start up the application task
 */
void bt_app_task_start_up(void);

/**
 * @brief    shut down the application task
 */
void bt_app_task_shut_down(void);

void bt_audio_task(void);

void bt_user_select_device(int index);

#endif /* __BT_AUDIO_H__ */
