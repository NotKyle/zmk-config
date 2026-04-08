/*
 * custom_status_screen.c
 *
 * Both halves use nice!view (160×68 LVGL, physically portrait 68×160).
 * Portrait rotation via lv_canvas_transform() angle=900 on 68×68 canvases
 * (same pattern as ZMK v0.3 built-in nice_view widget).
 *
 * Canvas coordinate → portrait coordinate:
 *   portrait_y = canvas_y          (within the canvas section)
 *   portrait_x = 67 - canvas_x     (x is mirrored; use centered text)
 *
 * Two canvases cover 136 of 160 portrait-height pixels:
 *   top canvas  (LV_ALIGN_TOP_RIGHT,      0, 0)  → portrait y=0..67
 *   mid canvas  (LV_ALIGN_TOP_LEFT,  +24, 0, 0)  → portrait y=68..135
 *
 * Left  half (central):  layer name · battery · WPM · connection type
 * Right half (peripheral): LILY58 title · battery · BLE status
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

#ifndef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#  include <zmk/events/split_peripheral_status_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_WPM)
#  include <zmk/wpm.h>
#  include <zmk/events/wpm_state_changed.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ── Layer names ─────────────────────────────────────────────────────────── */
static const char *const LAYER_NAMES[] = {
    "DEFAULT", "SYMBOLS", "NUMBERS", "NAV",
};
#define N_LAYERS ARRAY_SIZE(LAYER_NAMES)

/* ── Atomic state ─────────────────────────────────────────────────────────── */
static atomic_t a_layer   = ATOMIC_INIT(0);
static atomic_t a_battery = ATOMIC_INIT(100);
static atomic_t a_conn    = ATOMIC_INIT(0);  /* 0=none 1=partial 2=full */
#if IS_ENABLED(CONFIG_ZMK_WPM)
static atomic_t a_wpm     = ATOMIC_INIT(0);
#endif
static atomic_t a_dirty   = ATOMIC_INIT(1);

static void mark_dirty(void) { atomic_set(&a_dirty, 1); }

/* ── Canvas infrastructure ───────────────────────────────────────────────── */
#define CS 68

static lv_color_t cbuf_top[CS * CS];
static lv_color_t cbuf_mid[CS * CS];
static lv_obj_t  *canvas_top = NULL;
static lv_obj_t  *canvas_mid = NULL;

/* Rotate a CS×CS canvas 90° CW in-place (ZMK v0.3 pattern). */
static void canvas_rotate(lv_obj_t *canvas, lv_color_t cbuf[]) {
    static lv_color_t tmp[CS * CS];
    memcpy(tmp, cbuf, sizeof(tmp));
    lv_img_dsc_t img;
    img.data      = (void *)tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w  = CS;
    img.header.h  = CS;
    img.data_size = sizeof(tmp);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_canvas_transform(canvas, &img, 900, LV_IMG_ZOOM_NONE,
                        -1, 0, CS / 2, CS / 2, true);
}

/* ── Per-half draw ───────────────────────────────────────────────────────── */
static void draw_display(void) {
    if (!canvas_top || !canvas_mid) return;

    lv_draw_label_dsc_t lbl;
    lv_draw_rect_dsc_t  rect;

#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
    /* ── Left half: layer name · battery · WPM · connection ─────────── */

    /* TOP canvas: layer name (portrait y=0..67) */
    lv_canvas_fill_bg(canvas_top, lv_color_black(), LV_OPA_COVER);

    lv_draw_label_dsc_init(&lbl);
    lbl.color = lv_color_white();
    lbl.font  = &lv_font_montserrat_14;
    lbl.align = LV_TEXT_ALIGN_CENTER;

    int idx = (int)atomic_get(&a_layer);
    const char *layer_name = (idx >= 0 && idx < (int)N_LAYERS)
                             ? LAYER_NAMES[idx] : "???";
    lv_canvas_draw_text(canvas_top, 0, 8, CS, &lbl, layer_name);

    /* Separator */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    lv_canvas_draw_rect(canvas_top, 4, 30, CS - 8, 1, &rect);

    canvas_rotate(canvas_top, cbuf_top);

    /* MID canvas: battery + WPM + connection (portrait y=68..135) */
    lv_canvas_fill_bg(canvas_mid, lv_color_black(), LV_OPA_COVER);

    int bat = (int)atomic_get(&a_battery);

    /* Battery bar outline */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    lv_canvas_draw_rect(canvas_mid, 4, 6, CS - 8, 12, &rect);
    /* Inner background */
    rect.bg_color = lv_color_black();
    lv_canvas_draw_rect(canvas_mid, 5, 7, CS - 10, 10, &rect);
    /* Fill from right so it reads left→right in portrait */
    int fill = ((CS - 10) * bat) / 100;
    if (fill > 0) {
        rect.bg_color = lv_color_white();
        lv_canvas_draw_rect(canvas_mid, CS - 5 - fill, 7, fill, 10, &rect);
    }

    /* Battery % */
    static char bat_str[8];
    snprintf(bat_str, sizeof(bat_str), "%d%%", bat);
    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(canvas_mid, 0, 22, CS, &lbl, bat_str);

#  if IS_ENABLED(CONFIG_ZMK_WPM)
    static char wpm_str[12];
    snprintf(wpm_str, sizeof(wpm_str), "%d wpm", (int)atomic_get(&a_wpm));
    lv_canvas_draw_text(canvas_mid, 0, 36, CS, &lbl, wpm_str);
#  endif

    /* Connection type */
    int conn = (int)atomic_get(&a_conn);
    const char *conn_str = NULL;
    if (conn == 2) {
#  if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        conn_str = zmk_usb_is_hid_ready() ? "HID" : "USB";
#  else
        conn_str = "BLE";
#  endif
    } else if (conn == 1) {
        conn_str = "USB";
    }
    if (conn_str) {
#  if IS_ENABLED(CONFIG_ZMK_WPM)
        lv_canvas_draw_text(canvas_mid, 0, 50, CS, &lbl, conn_str);
#  else
        lv_canvas_draw_text(canvas_mid, 0, 36, CS, &lbl, conn_str);
#  endif
    }

    canvas_rotate(canvas_mid, cbuf_mid);

#else  /* peripheral */
    /* ── Right half: LILY58 title · battery · BLE status ────────────── */

    /* TOP canvas: title + battery (portrait y=0..67) */
    lv_canvas_fill_bg(canvas_top, lv_color_black(), LV_OPA_COVER);

    lv_draw_label_dsc_init(&lbl);
    lbl.color = lv_color_white();
    lbl.font  = &lv_font_montserrat_14;
    lbl.align = LV_TEXT_ALIGN_CENTER;
    lv_canvas_draw_text(canvas_top, 0, 5, CS, &lbl, "LILY58");

    /* Separator */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    lv_canvas_draw_rect(canvas_top, 4, 24, CS - 8, 1, &rect);

    /* Battery outline */
    lv_canvas_draw_rect(canvas_top, 4, 28, CS - 8, 12, &rect);
    /* Inner background */
    rect.bg_color = lv_color_black();
    lv_canvas_draw_rect(canvas_top, 5, 29, CS - 10, 10, &rect);
    /* Fill */
    int bat = (int)atomic_get(&a_battery);
    int fill = ((CS - 10) * bat) / 100;
    if (fill > 0) {
        rect.bg_color = lv_color_white();
        lv_canvas_draw_rect(canvas_top, CS - 5 - fill, 29, fill, 10, &rect);
    }

    /* Battery % */
    static char bat_str[8];
    snprintf(bat_str, sizeof(bat_str), "%d%%", bat);
    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(canvas_top, 0, 44, CS, &lbl, bat_str);

    canvas_rotate(canvas_top, cbuf_top);

    /* MID canvas: BLE status (portrait y=68..135) */
    lv_canvas_fill_bg(canvas_mid, lv_color_black(), LV_OPA_COVER);

    int conn = (int)atomic_get(&a_conn);

    lbl.font  = &lv_font_montserrat_14;
    lv_canvas_draw_text(canvas_mid, 0, 8, CS, &lbl, "BLE");

    /* Status dot */
    lv_draw_rect_dsc_init(&rect);
    rect.radius       = LV_RADIUS_CIRCLE;
    rect.border_color = lv_color_white();
    rect.border_width = 1;
    rect.border_opa   = LV_OPA_COVER;
    rect.bg_color     = (conn == 2) ? lv_color_white() : lv_color_black();
    rect.bg_opa       = LV_OPA_COVER;
    lv_canvas_draw_rect(canvas_mid, (CS - 10) / 2, 30, 10, 10, &rect);

    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(canvas_mid, 0, 46, CS, &lbl,
                        (conn == 2) ? "Connected" : "No signal");

    canvas_rotate(canvas_mid, cbuf_mid);
#endif
}

/* ── Connection state poller (central only) ──────────────────────────────── */
static void refresh_conn(void) {
#ifndef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
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
    if ((int)atomic_get(&a_conn) != s) {
        atomic_set(&a_conn, s);
        mark_dirty();
    }
#endif
}

/* ── LVGL timer callback ─────────────────────────────────────────────────── */
static void update_cb(lv_timer_t *t) {
    refresh_conn();
    if (!atomic_cas(&a_dirty, 1, 0)) return;
    draw_display();
}

/* ── ZMK event handlers ──────────────────────────────────────────────────── */

#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
static int on_layer_changed(const zmk_event_t *eh) {
    uint8_t top = 0;
    for (int i = (int)N_LAYERS - 1; i >= 1; i--)
        if (zmk_keymap_layer_active(i)) { top = (uint8_t)i; break; }
    atomic_set(&a_layer, top);
    mark_dirty();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_layer, on_layer_changed);
ZMK_SUBSCRIPTION(css_layer, zmk_layer_state_changed);
#endif

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

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
static int on_usb_changed(const zmk_event_t *eh) {
    refresh_conn(); return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_usb, on_usb_changed);
ZMK_SUBSCRIPTION(css_usb, zmk_usb_conn_state_changed);
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE) && defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static int on_ble_changed(const zmk_event_t *eh) {
    refresh_conn(); return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_ble, on_ble_changed);
ZMK_SUBSCRIPTION(css_ble, zmk_ble_active_profile_changed);
#endif

#ifndef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
static int on_periph_status_changed(const zmk_event_t *eh) {
    struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);
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
#endif

/* ── Screen constructor ───────────────────────────────────────────────────── */
lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    canvas_top = lv_canvas_create(scr);
    lv_obj_align(canvas_top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(canvas_top, cbuf_top, CS, CS, LV_IMG_CF_TRUE_COLOR);

    canvas_mid = lv_canvas_create(scr);
    lv_obj_align(canvas_mid, LV_ALIGN_TOP_LEFT, 24, 0);
    lv_canvas_set_buffer(canvas_mid, cbuf_mid, CS, CS, LV_IMG_CF_TRUE_COLOR);

    refresh_conn();
    atomic_set(&a_battery, 100);
    atomic_set(&a_dirty, 1);
    draw_display();

    lv_timer_create(update_cb, 500, NULL);
    return scr;
}
