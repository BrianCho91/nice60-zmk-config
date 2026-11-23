/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT nice60_behavior_persistent_os_toggle

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_REGISTER(nice60_os_mode, LOG_LEVEL_INF);

#define MAC_LAYER 1
#define SETTINGS_KEY "nice60_os/mac"

static uint8_t saved_mac_mode;
static struct k_work_delayable restore_work;

/* Restore the saved mode after ZMK has initialized its keymap. */
static void restore_mode(struct k_work *work) {
    if (saved_mac_mode) {
        int ret = zmk_keymap_layer_activate(MAC_LAYER, true);
        if (ret < 0) {
            LOG_ERR("Failed to restore Mac mode: %d", ret);
        }
    }
}

/* Read the persistent mode from Zephyr's settings storage. */
static int os_settings_set(const char *name, size_t len,
                           settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "mac") != 0 || len != sizeof(saved_mac_mode)) {
        return -ENOENT;
    }
    int ret = read_cb(cb_arg, &saved_mac_mode, sizeof(saved_mac_mode));
    return ret < 0 ? ret : 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(nice60_os, "nice60_os", NULL,
                               os_settings_set, NULL, NULL);

/* Save the new mode only when the user explicitly switches OS. */
static int os_toggle_pressed(struct zmk_behavior_binding *binding,
                             struct zmk_behavior_binding_event event) {
    bool enable_mac = !zmk_keymap_layer_active(MAC_LAYER);
    int ret = enable_mac
        ? zmk_keymap_layer_activate(MAC_LAYER, true)
        : zmk_keymap_layer_deactivate(MAC_LAYER, true);
    if (ret < 0) {
        return ret;
    }
    saved_mac_mode = enable_mac ? 1 : 0;
    ret = settings_save_one(SETTINGS_KEY, &saved_mac_mode, sizeof(saved_mac_mode));
    if (ret < 0) {
        LOG_ERR("Could not save OS mode: %d", ret);
    }
    return ret;
}

static int os_toggle_released(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api os_toggle_api = {
    .binding_pressed = os_toggle_pressed,
    .binding_released = os_toggle_released,
};

#define OS_TOGGLE_INST(n) \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL, \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &os_toggle_api);
DT_INST_FOREACH_STATUS_OKAY(OS_TOGGLE_INST)

static int os_mode_init(void) {
    k_work_init_delayable(&restore_work, restore_mode);
    k_work_schedule(&restore_work, K_MSEC(750));
    return 0;
}
SYS_INIT(os_mode_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY + 1);
