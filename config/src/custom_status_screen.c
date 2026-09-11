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
 * Left  half (central):  layer name · held modifiers · battery · WPM · link
 * Right half (peripheral): battery · link to central
 *
 * A ZMK peripheral has no access to layer or WPM state, so the right half is
 * limited to what it can actually see about itself.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <dt-bindings/zmk/modifiers.h>

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

/* ── Atomic state ─────────────────────────────────────────────────────────── */
static atomic_t a_layer   = ATOMIC_INIT(0);
static atomic_t a_battery = ATOMIC_INIT(100);
static atomic_t a_conn    = ATOMIC_INIT(0);  /* 0=none 1=partial 2=full */
static atomic_t a_mods    = ATOMIC_INIT(0);  /* zmk_mod_flags_t bitmask */
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
    /* ── Left half: layer · mods · battery · WPM · link ─────────────── */

    /* TOP canvas: layer name + held modifiers (portrait y=0..67) */
    lv_canvas_fill_bg(canvas_top, lv_color_black(), LV_OPA_COVER);

    lv_draw_label_dsc_init(&lbl);
    lbl.color = lv_color_white();
    lbl.font  = &lv_font_montserrat_14;
    lbl.align = LV_TEXT_ALIGN_CENTER;

    /* Name comes straight from the keymap's display-name, so adding a layer
     * needs no change here. */
    zmk_keymap_layer_id_t id =
        zmk_keymap_layer_index_to_id((zmk_keymap_layer_index_t)atomic_get(&a_layer));
    const char *layer_name = zmk_keymap_layer_name(id);
    lv_canvas_draw_text(canvas_top, 0, 6, CS, &lbl, layer_name ? layer_name : "?");

    /* Separator */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    lv_canvas_draw_rect(canvas_top, 4, 28, CS - 8, 1, &rect);

    /* Held modifiers, active ones only: G=gui A=alt C=ctrl S=shift.
     * Drawn as one centred string so it needs no portrait mirroring. */
    uint8_t mods = (uint8_t)atomic_get(&a_mods);
    if (mods) {
        static char mod_str[10];
        int n = 0;
        if (mods & (MOD_LGUI | MOD_RGUI)) { mod_str[n++] = 'G'; mod_str[n++] = ' '; }
        if (mods & (MOD_LALT | MOD_RALT)) { mod_str[n++] = 'A'; mod_str[n++] = ' '; }
        if (mods & (MOD_LCTL | MOD_RCTL)) { mod_str[n++] = 'C'; mod_str[n++] = ' '; }
        if (mods & (MOD_LSFT | MOD_RSFT)) { mod_str[n++] = 'S'; mod_str[n++] = ' '; }
        if (n) { mod_str[n - 1] = 0; }   /* drop the trailing space */
        lbl.font = &lv_font_montserrat_14;
        lv_canvas_draw_text(canvas_top, 0, 38, CS, &lbl, mod_str);
    }

    canvas_rotate(canvas_top, cbuf_top);

    /* MID canvas: battery + WPM + connection (portrait y=68..135) */
    lv_canvas_fill_bg(canvas_mid, lv_color_black(), LV_OPA_COVER);

    int bat = (int)atomic_get(&a_battery);

    /* Battery bar outline */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    lv_canvas_draw_rect(canvas_mid, 4, 4, CS - 8, 12, &rect);
    /* Inner background */
    rect.bg_color = lv_color_black();
    lv_canvas_draw_rect(canvas_mid, 5, 5, CS - 10, 10, &rect);
    /* Fill from right so it reads left→right in portrait */
    int fill = ((CS - 10) * bat) / 100;
    if (fill > 0) {
        rect.bg_color = lv_color_white();
        lv_canvas_draw_rect(canvas_mid, CS - 5 - fill, 5, fill, 10, &rect);
    }

    /* Battery % */
    static char bat_str[8];
    snprintf(bat_str, sizeof(bat_str), "%d%%", bat);
    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(canvas_mid, 0, 17, CS, &lbl, bat_str);

#  if IS_ENABLED(CONFIG_ZMK_WPM)
    /* WPM as a bar as well as a number — the bar is what you can read at a
     * glance. Full scale is 100 wpm; faster than that just pins it. */
    int wpm = (int)atomic_get(&a_wpm);
    int wfill = ((CS - 10) * (wpm > 100 ? 100 : wpm)) / 100;
    rect.bg_color = lv_color_white();
    lv_canvas_draw_rect(canvas_mid, 4, 31, CS - 8, 1, &rect);   /* baseline */
    if (wfill > 0) {
        lv_canvas_draw_rect(canvas_mid, CS - 5 - wfill, 32, wfill, 6, &rect);
    }

    static char wpm_str[12];
    snprintf(wpm_str, sizeof(wpm_str), "%d wpm", wpm);
    lv_canvas_draw_text(canvas_mid, 0, 40, CS, &lbl, wpm_str);
#  endif

    /* Connection type — only show when there's an actual input connection */
    int conn = (int)atomic_get(&a_conn);
    const char *conn_str = NULL;
    if (conn == 2) {
#  if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        conn_str = zmk_usb_is_hid_ready() ? "HID" : "BLE";
#  else
        conn_str = "BLE";
#  endif
    }
    if (conn_str) {
#  if IS_ENABLED(CONFIG_ZMK_WPM)
        lv_canvas_draw_text(canvas_mid, 0, 53, CS, &lbl, conn_str);
#  else
        lv_canvas_draw_text(canvas_mid, 0, 31, CS, &lbl, conn_str);
#  endif
    }

    canvas_rotate(canvas_mid, cbuf_mid);

#else  /* peripheral */
    /* ── Right half: battery · link to central ──────────────────────── */
    /* A peripheral sees neither layer nor WPM state, so there is nothing
     * else honest to put here. Battery gets the space instead of a title. */

    /* TOP canvas: battery, large (portrait y=0..67) */
    lv_canvas_fill_bg(canvas_top, lv_color_black(), LV_OPA_COVER);

    lv_draw_label_dsc_init(&lbl);
    lbl.color = lv_color_white();
    lbl.align = LV_TEXT_ALIGN_CENTER;

    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;

    int bat = (int)atomic_get(&a_battery);

    /* Outline */
    lv_canvas_draw_rect(canvas_top, 4, 8, CS - 8, 16, &rect);
    /* Inner background */
    rect.bg_color = lv_color_black();
    lv_canvas_draw_rect(canvas_top, 5, 9, CS - 10, 14, &rect);
    /* Fill from right so it reads left→right in portrait */
    int fill = ((CS - 10) * bat) / 100;
    if (fill > 0) {
        rect.bg_color = lv_color_white();
        lv_canvas_draw_rect(canvas_top, CS - 5 - fill, 9, fill, 14, &rect);
    }

    static char bat_str[8];
    snprintf(bat_str, sizeof(bat_str), "%d%%", bat);
    lbl.font = &lv_font_montserrat_14;
    lv_canvas_draw_text(canvas_top, 0, 28, CS, &lbl, bat_str);

    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(canvas_top, 0, 48, CS, &lbl, "RIGHT");

    canvas_rotate(canvas_top, cbuf_top);

    /* MID canvas: link to the central half (portrait y=68..135) */
    lv_canvas_fill_bg(canvas_mid, lv_color_black(), LV_OPA_COVER);

    int conn = (int)atomic_get(&a_conn);
    bool linked = (conn == 2);

    /* A filled bar reads as "joined", a broken one as "split". */
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = lv_color_white();
    rect.radius   = 0;
    if (linked) {
        lv_canvas_draw_rect(canvas_mid, 8, 16, CS - 16, 4, &rect);
    } else {
        lv_canvas_draw_rect(canvas_mid, 8, 16, 18, 4, &rect);
        lv_canvas_draw_rect(canvas_mid, CS - 26, 16, 18, 4, &rect);
    }

    lbl.font = &lv_font_montserrat_10;
    lv_canvas_draw_text(canvas_mid, 0, 28, CS, &lbl, linked ? "linked" : "no link");

    canvas_rotate(canvas_mid, cbuf_mid);
#endif
}

/* ── Connection state poller (central only) ──────────────────────────────── */
static void refresh_conn(void) {
#ifndef CONFIG_ZMK_SPLIT_ROLE_CENTRAL
    return;
#else
    int s = 0;
    /* USB HID takes priority; USB-powered-only falls through to BLE check. */
#  if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    if (zmk_usb_get_conn_state() == ZMK_USB_CONN_HID) {
        s = 2;
    }
#  endif
#  if IS_ENABLED(CONFIG_ZMK_BLE)
    if (s == 0 && zmk_ble_active_profile_is_connected()) {
        s = 2;
    }
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
    atomic_set(&a_layer, (atomic_val_t)zmk_keymap_highest_layer_active());
    mark_dirty();
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_layer, on_layer_changed);
ZMK_SUBSCRIPTION(css_layer, zmk_layer_state_changed);

/* Modifiers are read off the HID report on every keycode event rather than
 * polled, so the indicator tracks the key rather than lagging the timer. */
static int on_keycode_changed(const zmk_event_t *eh) {
    atomic_val_t mods = (atomic_val_t)zmk_hid_get_explicit_mods();
    if (atomic_get(&a_mods) != mods) {
        atomic_set(&a_mods, mods);
        mark_dirty();
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(css_mods, on_keycode_changed);
ZMK_SUBSCRIPTION(css_mods, zmk_keycode_state_changed);
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
