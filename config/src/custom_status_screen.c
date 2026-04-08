/*
 * custom_status_screen.c
 *
 * Left half  (central, SSD1306 128×64, landscape widget layout)
 * Right half (peripheral, nice!view 160×68, portrait via canvas rotation)
 *
 * Portrait mechanism (right half):
 *   The nice!view is 160×68 in LVGL but physically mounted portrait (68×160).
 *   We draw onto 68×68 lv_color_t canvases then call lv_canvas_transform()
 *   with angle=900 (90° CW) — the same pattern ZMK v0.3 uses internally.
 *   Canvas ys maps directly to portrait y within the section (0=top, 67=bot).
 *   Canvas xs maps to portrait x=67-xs (mirrored); use centered text.
 *
 *   Two sections cover 136 of 160 portrait-height pixels:
 *     top canvas (LV_ALIGN_TOP_RIGHT)     → portrait y=0..67
 *     mid canvas (LV_ALIGN_TOP_LEFT +24)  → portrait y=68..135
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

/* ── Atomic state (event handlers write; LVGL timer reads) ───────────────── */
static atomic_t a_layer   = ATOMIC_INIT(0);
static atomic_t a_battery = ATOMIC_INIT(100);
static atomic_t a_conn    = ATOMIC_INIT(0);   /* 0=none 1=partial 2=full */
#if IS_ENABLED(CONFIG_ZMK_WPM)
static atomic_t a_wpm     = ATOMIC_INIT(0);
#endif
static atomic_t a_dirty   = ATOMIC_INIT(1);

static void mark_dirty(void) { atomic_set(&a_dirty, 1); }

/* ─────────────────────────────────────────────────────────────────────────
 * LEFT HALF (central): LVGL widget objects
 * ───────────────────────────────────────────────────────────────────────── */
#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
static lv_obj_t *layer_lbl = NULL;
static lv_obj_t *conn_lbl  = NULL;
static lv_obj_t *conn_dot  = NULL;
static lv_obj_t *bat_bar   = NULL;
static lv_obj_t *bat_lbl   = NULL;
#if IS_ENABLED(CONFIG_ZMK_WPM)
static lv_obj_t *wpm_lbl   = NULL;
#endif
#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */

/* ─────────────────────────────────────────────────────────────────────────
 * RIGHT HALF (peripheral): canvas buffers + handles
 * ───────────────────────────────────────────────────────────────────────── */
#ifndef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#define CS 68

static lv_color_t r_cbuf_top[CS * CS];
static lv_color_t r_cbuf_mid[CS * CS];
static lv_obj_t  *r_canvas_top = NULL;
static lv_obj_t  *r_canvas_mid = NULL;

/* Rotate a CS×CS canvas 90° CW (ZMK v0.3 pattern). */
static void r_rotate(lv_obj_t *canvas, lv_color_t cbuf[]) {
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

/* Redraw and rotate both portrait canvases. */
static void r_draw(void) {
    if (!r_canvas_top || !r_canvas_mid) return;

    lv_draw_rect_dsc_t rect;
    lv_draw_label_dsc_t lbl;

    /* ── TOP canvas: "LILY58" + battery (portrait y=0..67) ──────────── */
    lv_canvas_fill_bg(r_canvas_top, lv_color_black(), LV_OPA_COVER);

    lv_draw_label_dsc_init(&lbl);
    lbl.color = lv_color_white();
    lbl.font  = &lv_font_montserrat_14;
    lbl.align = LV_TEXT_ALIGN_CENTER;
    lv_canvas_draw_text(r_canvas_top, 0, 5, CS, &lbl, "LILY58");

    /* Separator */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    lv_canvas_draw_rect(r_canvas_top, 4, 24, CS - 8, 1, &rect);

    /* Battery outline */
    lv_canvas_draw_rect(r_canvas_top, 4, 28, CS - 8, 12, &rect);
    /* Inner background */
    rect.bg_color = lv_color_black();
    lv_canvas_draw_rect(r_canvas_top, 5, 29, CS - 10, 10, &rect);
    /* Fill: drawn from canvas right so it reads left→right in portrait */
    int bat = (int)atomic_get(&a_battery);
    int fill = ((CS - 10) * bat) / 100;
    if (fill > 0) {
        rect.bg_color = lv_color_white();
        lv_canvas_draw_rect(r_canvas_top, CS - 5 - fill, 29, fill, 10, &rect);
    }

    /* Battery % */
    static char bat_str[8];
    snprintf(bat_str, sizeof(bat_str), "%d%%", bat);
    lbl.font  = &lv_font_montserrat_10;
    lv_canvas_draw_text(r_canvas_top, 0, 44, CS, &lbl, bat_str);

    r_rotate(r_canvas_top, r_cbuf_top);

    /* ── MID canvas: BLE status (portrait y=68..135) ─────────────────── */
    lv_canvas_fill_bg(r_canvas_mid, lv_color_black(), LV_OPA_COVER);

    int conn = (int)atomic_get(&a_conn);

    lbl.font  = &lv_font_montserrat_14;
    lv_canvas_draw_text(r_canvas_mid, 0, 8, CS, &lbl, "BLE");

    /* Status dot: filled=connected, outline=not */
    lv_draw_rect_dsc_init(&rect);
    rect.radius        = LV_RADIUS_CIRCLE;
    rect.border_color  = lv_color_white();
    rect.border_width  = 1;
    rect.border_opa    = LV_OPA_COVER;
    rect.bg_color      = (conn == 2) ? lv_color_white() : lv_color_black();
    rect.bg_opa        = LV_OPA_COVER;
    lv_canvas_draw_rect(r_canvas_mid, (CS - 10) / 2, 30, 10, 10, &rect);

    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(r_canvas_mid, 0, 46, CS, &lbl,
                        (conn == 2) ? "Connected" : "No signal");

    r_rotate(r_canvas_mid, r_cbuf_mid);
}
#endif /* !CONFIG_ZMK_SPLIT_ROLE_CENTRAL */

/* ── Connection state poller (central only; peripheral is event-driven) ─── */
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

#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
    /* Layer */
    if (layer_lbl) {
        int idx = (int)atomic_get(&a_layer);
        if (idx >= 0 && idx < (int)N_LAYERS)
            lv_label_set_text(layer_lbl, LAYER_NAMES[idx]);
    }

    /* Battery */
    int bat = (int)atomic_get(&a_battery);
    if (bat_bar) lv_bar_set_value(bat_bar, bat, LV_ANIM_OFF);
    if (bat_lbl) {
        static char bat_buf[8];
        snprintf(bat_buf, sizeof(bat_buf), "%d%%", bat);
        lv_label_set_text(bat_lbl, bat_buf);
    }

    /* WPM */
#  if IS_ENABLED(CONFIG_ZMK_WPM)
    if (wpm_lbl) {
        static char wpm_buf[12];
        snprintf(wpm_buf, sizeof(wpm_buf), "%d wpm", (int)atomic_get(&a_wpm));
        lv_label_set_text(wpm_lbl, wpm_buf);
    }
#  endif

    /* Connection */
    int conn = (int)atomic_get(&a_conn);
    if (conn_lbl) {
        if (conn == 0) {
            lv_obj_add_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);
        } else {
            const char *t_str = "BLE";
#  if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
            if (conn == 2 && zmk_usb_is_hid_ready()) t_str = "HID";
            else if (conn == 1)                       t_str = "USB";
#  endif
            lv_label_set_text(conn_lbl, t_str);
            lv_obj_clear_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (conn_dot) {
        if (conn == 0) {
            lv_obj_add_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_style_bg_opa(conn_dot,
                conn == 2 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_color(conn_dot, lv_color_black(), 0);
            lv_obj_set_style_border_opa(conn_dot, LV_OPA_COVER, 0);
            lv_obj_clear_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }

#else  /* peripheral */
    r_draw();
#endif
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

/* ── Helpers ─────────────────────────────────────────────────────────────── */
static void style_remove_defaults(lv_obj_t *obj) {
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

/* ── Screen constructor ───────────────────────────────────────────────────── */
lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    style_remove_defaults(scr);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

#ifdef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
    /* ── Left half: landscape widgets on white background ───────────── */
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    layer_lbl = lv_label_create(scr);
    style_remove_defaults(layer_lbl);
    lv_label_set_text(layer_lbl, "DEFAULT");
    lv_label_set_long_mode(layer_lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(layer_lbl, LV_HOR_RES - 8);
    lv_obj_set_style_text_font(layer_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(layer_lbl, lv_color_black(), 0);
    lv_obj_set_style_text_align(layer_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(layer_lbl, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *sep = lv_obj_create(scr);
    style_remove_defaults(sep);
    lv_obj_set_size(sep, LV_HOR_RES - 8, 1);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(sep, lv_color_black(), 0);
    lv_obj_align(sep, LV_ALIGN_TOP_LEFT, 4, 26);

    bat_bar = lv_bar_create(scr);
    lv_obj_set_size(bat_bar, LV_HOR_RES - 8, 12);
    lv_bar_set_range(bat_bar, 0, 100);
    lv_bar_set_value(bat_bar, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(bat_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bat_bar, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_border_color(bat_bar, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(bat_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(bat_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bat_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bat_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bat_bar, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bat_bar, 1, LV_PART_INDICATOR);
    lv_obj_align(bat_bar, LV_ALIGN_TOP_LEFT, 4, 32);

    bat_lbl = lv_label_create(scr);
    style_remove_defaults(bat_lbl);
    lv_label_set_text(bat_lbl, "100%");
    lv_obj_set_style_text_font(bat_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(bat_lbl, lv_color_black(), 0);
    lv_obj_align_to(bat_lbl, bat_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

#  if IS_ENABLED(CONFIG_ZMK_WPM)
    wpm_lbl = lv_label_create(scr);
    style_remove_defaults(wpm_lbl);
    lv_label_set_text(wpm_lbl, "0 wpm");
    lv_obj_set_style_text_font(wpm_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(wpm_lbl, lv_color_black(), 0);
    lv_obj_align(wpm_lbl, LV_ALIGN_TOP_MID, 0, 62);
#  endif

    conn_lbl = lv_label_create(scr);
    style_remove_defaults(conn_lbl);
    lv_label_set_text(conn_lbl, "HID");
    lv_obj_set_style_text_font(conn_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(conn_lbl, lv_color_black(), 0);
    lv_obj_align(conn_lbl, LV_ALIGN_BOTTOM_LEFT, 4, -5);
    lv_obj_add_flag(conn_lbl, LV_OBJ_FLAG_HIDDEN);

    conn_dot = lv_obj_create(scr);
    style_remove_defaults(conn_dot);
    lv_obj_set_size(conn_dot, 7, 7);
    lv_obj_set_style_radius(conn_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_color(conn_dot, lv_color_black(), 0);
    lv_obj_set_style_border_width(conn_dot, 1, 0);
    lv_obj_set_style_border_opa(conn_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(conn_dot, LV_OPA_TRANSP, 0);
    lv_obj_align(conn_dot, LV_ALIGN_BOTTOM_RIGHT, -4, -5);
    lv_obj_add_flag(conn_dot, LV_OBJ_FLAG_HIDDEN);

#else
    /* ── Right half: portrait canvases on black background ──────────── */
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    r_canvas_top = lv_canvas_create(scr);
    lv_obj_align(r_canvas_top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(r_canvas_top, r_cbuf_top, CS, CS, LV_IMG_CF_TRUE_COLOR);

    r_canvas_mid = lv_canvas_create(scr);
    lv_obj_align(r_canvas_mid, LV_ALIGN_TOP_LEFT, 24, 0);
    lv_canvas_set_buffer(r_canvas_mid, r_cbuf_mid, CS, CS, LV_IMG_CF_TRUE_COLOR);

    r_draw();
#endif

    lv_timer_create(update_cb, 500, NULL);
    refresh_conn();
    atomic_set(&a_battery, 100);
    atomic_set(&a_dirty, 1);

    return scr;
}
