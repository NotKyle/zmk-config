/*
 * custom_status_screen.c
 *
 * Custom nice!view status screen for the Lily58 (160×68 px, monochrome).
 * Provides zmk_display_status_screen() which ZMK calls at display init.
 *
 * Layout:
 *   ┌────────────────────────────────────────────────────────┐
 *   │  LAYER NAME                               HID  ●      │  ← 18 px
 *   │  ──────────────────────────────────────────────────    │  ← 1 px
 *   │  ████████████████████████░░░░░░░░░   87%              │  ← 12 px
 *   │                                                        │  ← gap
 *   │  42 wpm                                               │  ← 12 px
 *   └────────────────────────────────────────────────────────┘
 *
 * Thread model:
 *   ZMK event handlers run outside the LVGL thread — they only update
 *   atomic variables. An LVGL timer reads those atomics inside the LVGL
 *   thread and applies changes to widgets. No external locking needed.
 *
 * Left half  (central, USB host):
 *   conn = USB HID ready → "HID ●"
 *   conn = USB powered but not claimed → "USB ○"
 *   conn = BLE profile active → "BLE ●"
 *   conn = none → hidden
 *
 * Right half (peripheral, BLE split):
 *   conn = connected to central → "BLE ●"
 *   conn = disconnected → hidden
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#if IS_ENABLED(CONFIG_ZMK_BATTERY)
#  include <zmk/battery.h>
#  include <zmk/events/battery_state_changed.h>
#endif

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#  include <zmk/usb.h>
#  include <zmk/events/usb_conn_state_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
#  include <zmk/ble.h>
#  include <zmk/events/ble_active_profile_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
#  include <zmk/events/split_peripheral_status_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_WPM)
#  include <zmk/wpm.h>
#  include <zmk/events/wpm_state_changed.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Layer names (match display-name in lily58.keymap) ───────────────────── */
static const char *const LAYER_NAMES[] = {
    "DEFAULT", "SYMBOLS", "NUMBERS", "NAV",
};
#define N_LAYERS ARRAY_SIZE(LAYER_NAMES)

/* ── Atomic state (written by event handlers, read by LVGL timer) ─────────── */
static atomic_t a_layer   = ATOMIC_INIT(0);   /* active layer index          */
static atomic_t a_battery = ATOMIC_INIT(100); /* state-of-charge 0-100       */
static atomic_t a_conn    = ATOMIC_INIT(0);   /* 0=none 1=partial 2=full     */
static atomic_t a_wpm     = ATOMIC_INIT(0);   /* words per minute            */
static atomic_t a_dirty   = ATOMIC_INIT(1);   /* set to 1 to trigger redraw  */

/* ── LVGL widget handles ──────────────────────────────────────────────────── */
static lv_obj_t *layer_lbl  = NULL; /* large layer name                      */
static lv_obj_t *conn_lbl   = NULL; /* "HID" / "BLE" / "USB" text            */
static lv_obj_t *conn_dot   = NULL; /* small filled/outlined circle           */
static lv_obj_t *bat_bar    = NULL; /* lv_bar 0-100                           */
static lv_obj_t *bat_lbl    = NULL; /* "87%"                                  */
static lv_obj_t *wpm_lbl    = NULL; /* "42 wpm"                              */

/* ── Forward declarations ─────────────────────────────────────────────────── */
static void refresh_conn(void);

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static void style_remove_defaults(lv_obj_t *obj) {
    lv_obj_remove_style_all(obj);
    /* Re-apply a clean transparent background so parent shows through. */
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

/* ── LVGL timer: apply atomic state to widgets ───────────────────────────── */
static void update_cb(lv_timer_t *t) {
    /* Always re-poll hardware state — events may fire before display init */
    refresh_conn();
    if (!atomic_cas(&a_dirty, 1, 0)) return;

    /* Layer label */
    if (layer_lbl) {
        int idx = (int)atomic_get(&a_layer);
        if (idx >= 0 && idx < (int)N_LAYERS) {
            lv_label_set_text(layer_lbl, LAYER_NAMES[idx]);
        }
    }

    /* Battery bar + label */
    int bat = (int)atomic_get(&a_battery);
    if (bat_bar)  lv_bar_set_value(bat_bar, bat, LV_ANIM_OFF);
    if (bat_lbl) {
        static char bat_buf[8];
        snprintf(bat_buf, sizeof(bat_buf), "%d%%", bat);
        lv_label_set_text(bat_lbl, bat_buf);
    }

    /* WPM label */
#if IS_ENABLED(CONFIG_ZMK_WPM)
    if (wpm_lbl) {
        static char wpm_buf[12];
        snprintf(wpm_buf, sizeof(wpm_buf), "%d wpm", (int)atomic_get(&a_wpm));
        lv_label_set_text(wpm_lbl, wpm_buf);
    }
#endif

    /* Connection indicator */
    int conn = (int)atomic_get(&a_conn);
    if (conn_lbl) {
        switch (conn) {
        case 2:
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
            lv_label_set_text(conn_lbl, "BLE");
#else
            /* Central: prefer to show transport type */
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
            if (zmk_usb_is_hid_ready())
                lv_label_set_text(conn_lbl, "HID");
            else
#endif
                lv_label_set_text(conn_lbl, "BLE");
#endif
            lv_obj_clear_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);
            break;
        case 1:
            lv_label_set_text(conn_lbl, "USB");
            lv_obj_clear_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);
            break;
        default:
            lv_obj_add_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);
            break;
        }
    }

    if (conn_dot) {
        if (conn == 2) {
            /* Filled: fully connected */
            lv_obj_set_style_bg_opa(conn_dot, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(conn_dot, lv_color_black(), 0);
            lv_obj_set_style_border_opa(conn_dot, LV_OPA_COVER, 0);
            lv_obj_clear_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);
        } else if (conn == 1) {
            /* Outline only: partial (USB powered, HID not claimed) */
            lv_obj_set_style_bg_opa(conn_dot, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_opa(conn_dot, LV_OPA_COVER, 0);
            lv_obj_clear_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            /* Hidden: not connected */
            lv_obj_add_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/* ── ZMK event handlers ──────────────────────────────────────────────────── */

static void mark_dirty(void) { atomic_set(&a_dirty, 1); }

/* Layer tracking is central-only; peripheral has no keymap API. */
#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
static int on_layer_changed(const zmk_event_t *eh) {
    /* Walk down from highest layer; first active one wins. */
    uint8_t top = 0;
    for (int i = (int)N_LAYERS - 1; i >= 1; i--) {
        if (zmk_keymap_layer_active(i)) { top = (uint8_t)i; break; }
    }
    atomic_set(&a_layer, top);
    mark_dirty();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_layer, on_layer_changed);
ZMK_SUBSCRIPTION(css_layer, zmk_layer_state_changed);
#endif /* !CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL */

#if IS_ENABLED(CONFIG_ZMK_BATTERY)
static int on_battery_changed(const zmk_event_t *eh) {
    atomic_set(&a_battery, (atomic_val_t)zmk_battery_state_of_charge());
    mark_dirty();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_battery, on_battery_changed);
ZMK_SUBSCRIPTION(css_battery, zmk_battery_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_WPM)
static int on_wpm_changed(const zmk_event_t *eh) {
    struct zmk_wpm_state_changed *ev = as_zmk_wpm_state_changed(eh);
    if (ev) { atomic_set(&a_wpm, ev->state); mark_dirty(); }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_wpm, on_wpm_changed);
ZMK_SUBSCRIPTION(css_wpm, zmk_wpm_state_changed);
#endif

static void refresh_conn(void) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
    /* Peripheral: connection state is driven by on_periph_status_changed; nothing to poll. */
    return;
#else
    int s = 0;
#  if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    switch (zmk_usb_get_conn_state()) {
    case ZMK_USB_CONN_HID:     s = 2; break;
    case ZMK_USB_CONN_POWERED: s = 1; break;
    default:
#    if IS_ENABLED(CONFIG_ZMK_BLE)
        s = zmk_ble_active_profile_is_connected() ? 2 : 0;
#    endif
        break;
    }
#  elif IS_ENABLED(CONFIG_ZMK_BLE)
    s = zmk_ble_active_profile_is_connected() ? 2 : 0;
#  endif
    /* Only mark dirty on change to avoid forcing a redraw every 500 ms */
    if ((int)atomic_get(&a_conn) != s) {
        atomic_set(&a_conn, s);
        mark_dirty();
    }
#endif /* !CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL */
}

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
static int on_usb_changed(const zmk_event_t *eh) {
    refresh_conn(); return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_usb, on_usb_changed);
ZMK_SUBSCRIPTION(css_usb, zmk_usb_conn_state_changed);
#endif

/* BLE active-profile event only exists on the central side. */
#if IS_ENABLED(CONFIG_ZMK_BLE) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
static int on_ble_changed(const zmk_event_t *eh) {
    refresh_conn(); return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_ble, on_ble_changed);
ZMK_SUBSCRIPTION(css_ble, zmk_ble_active_profile_changed);
#endif

/* Peripheral: track connection to the central via split peripheral status event. */
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL)
static int on_periph_status_changed(const zmk_event_t *eh) {
    struct zmk_split_peripheral_status_changed *ev = as_zmk_split_peripheral_status_changed(eh);
    if (ev) {
        int s = ev->connected ? 2 : 0;
        if ((int)atomic_get(&a_conn) != s) {
            atomic_set(&a_conn, s);
            mark_dirty();
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_periph, on_periph_status_changed);
ZMK_SUBSCRIPTION(css_periph, zmk_split_peripheral_status_changed);
#endif /* CONFIG_ZMK_SPLIT_ROLE_PERIPHERAL */

/* ── Screen constructor ──────────────────────────────────────────────────── */
lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    style_remove_defaults(scr);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    /* ── Row 0: layer name (left) + connection (right) ─────────────────── */
    layer_lbl = lv_label_create(scr);
    style_remove_defaults(layer_lbl);
    lv_label_set_text(layer_lbl, "DEFAULT");
    lv_obj_set_style_text_font(layer_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(layer_lbl, lv_color_black(), 0);
    lv_obj_align(layer_lbl, LV_ALIGN_TOP_LEFT, 4, 3);

    /* Connection label ("HID", "BLE", "USB") */
    conn_lbl = lv_label_create(scr);
    style_remove_defaults(conn_lbl);
    lv_label_set_text(conn_lbl, "HID");
    lv_obj_set_style_text_font(conn_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(conn_lbl, lv_color_black(), 0);
    lv_obj_align(conn_lbl, LV_ALIGN_TOP_RIGHT, -14, 5);
    lv_obj_add_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);

    /* Connection dot: 7×7 filled circle */
    conn_dot = lv_obj_create(scr);
    style_remove_defaults(conn_dot);
    lv_obj_set_size(conn_dot, 7, 7);
    lv_obj_set_style_radius(conn_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(conn_dot, lv_color_black(), 0);
    lv_obj_set_style_border_width(conn_dot, 1, 0);
    lv_obj_set_style_border_opa(conn_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(conn_dot, LV_OPA_TRANSP, 0);
    lv_obj_align(conn_dot, LV_ALIGN_TOP_RIGHT, -3, 5);
    lv_obj_add_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);

    /* ── Separator ─────────────────────────────────────────────────────── */
    lv_obj_t *sep = lv_obj_create(scr);
    style_remove_defaults(sep);
    lv_obj_set_size(sep, LV_HOR_RES - 8, 1);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(sep, lv_color_black(), 0);
    lv_obj_align(sep, LV_ALIGN_TOP_LEFT, 4, 23);

    /* ── Row 1: battery bar + percentage ──────────────────────────────── */
    bat_bar = lv_bar_create(scr);
    lv_obj_set_size(bat_bar, LV_HOR_RES - 52, 10);
    lv_bar_set_range(bat_bar, 0, 100);
    lv_bar_set_value(bat_bar, 100, LV_ANIM_OFF);
    /* Bar track */
    lv_obj_set_style_bg_opa(bat_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bat_bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(bat_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(bat_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(bat_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bat_bar, 1, LV_PART_MAIN);
    /* Bar fill */
    lv_obj_set_style_bg_opa(bat_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bat_bar, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bat_bar, 1, LV_PART_INDICATOR);
    lv_obj_align(bat_bar, LV_ALIGN_TOP_LEFT, 4, 30);

    bat_lbl = lv_label_create(scr);
    style_remove_defaults(bat_lbl);
    lv_label_set_text(bat_lbl, "100%");
    lv_obj_set_style_text_font(bat_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(bat_lbl, lv_color_black(), 0);
    lv_obj_align_to(bat_lbl, bat_bar, LV_ALIGN_OUT_RIGHT_MID, 4, 0);

    /* ── Row 2: WPM ────────────────────────────────────────────────────── */
#if IS_ENABLED(CONFIG_ZMK_WPM)
    wpm_lbl = lv_label_create(scr);
    style_remove_defaults(wpm_lbl);
    lv_label_set_text(wpm_lbl, "0 wpm");
    lv_obj_set_style_text_font(wpm_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(wpm_lbl, lv_color_black(), 0);
    lv_obj_align(wpm_lbl, LV_ALIGN_TOP_LEFT, 4, 50);
#endif

    /* Poll / redraw every 500 ms inside the LVGL thread */
    lv_timer_create(update_cb, 500, NULL);

    /* Seed initial state */
    refresh_conn();
    atomic_set(&a_battery, 100);
    atomic_set(&a_dirty, 1);

    return scr;
}
