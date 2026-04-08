/*
 * layer_reporter.c
 *
 * Broadcasts the currently active ZMK layer over the raw HID interface
 * whenever the layer state changes. The Hammerspoon HUD reads these packets
 * via the host-side Python sidecar (lily58-hid-sidecar.py).
 *
 * Packet format (32 bytes):
 *   [0]    = 0x01  (MSG_TYPE_LAYER)
 *   [1]    = layer index  (0 = default, 1 = symbols, 2 = numbers, 3 = nav)
 *   [2-31] = 0x00  (reserved)
 */

/* Raw HID is only available on the central (left) half. */
#if IS_ENABLED(CONFIG_RAW_HID)

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <raw_hid/events.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define MSG_TYPE_LAYER 0x01
#define REPORT_LEN     32

static int layer_state_changed_handler(const zmk_event_t *eh) {
    /* Walk layers 3 → 1; report the highest currently active one.
     * Layer 0 (default) is always active, so it's the fallback. */
    uint8_t layer = 0;
    for (int i = 3; i >= 1; i--) {
        if (zmk_keymap_layer_active(i)) {
            layer = (uint8_t)i;
            break;
        }
    }

    LOG_DBG("layer_reporter: active layer %d", layer);

    /* Static buffer — safe because ZMK events are processed sequentially. */
    static uint8_t report[REPORT_LEN];
    memset(report, 0, REPORT_LEN);
    report[0] = MSG_TYPE_LAYER;
    report[1] = layer;

    raise_raw_hid_sent_event((struct raw_hid_sent_event){
        .data   = report,
        .length = REPORT_LEN,
    });

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(layer_hid_reporter, layer_state_changed_handler);
ZMK_SUBSCRIPTION(layer_hid_reporter, zmk_layer_state_changed);

#endif /* IS_ENABLED(CONFIG_RAW_HID) */
