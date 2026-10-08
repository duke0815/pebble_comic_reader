// Prototyp (Schritt 1): fordert ein Test-Panel vom Handy an, zeigt es scrollbar an
// und blendet Groesse, Uebertragungsdauer und freien Heap ein.
// SELECT laedt das Panel erneut (zum Messen).
#include <pebble.h>
#include "transfer.h"

static Window *s_window;
static ScrollLayer *s_scroll_layer;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_bitmap;
static TextLayer *s_status_layer;
static char s_status_text[96];

static AppTimer *s_hide_timer;

static void hide_status(void *context) {
  s_hide_timer = NULL;
  layer_set_hidden(text_layer_get_layer(s_status_layer), true);
}

static void show_status(const char *text) {
  if (s_hide_timer) {
    app_timer_cancel(s_hide_timer);
    s_hide_timer = NULL;
  }
  text_layer_set_text(s_status_layer, text);
  layer_set_hidden(text_layer_get_layer(s_status_layer), text == NULL);
}

static void on_progress(uint32_t received, uint32_t total) {
  if (total == 0) {
    show_status("Lade Panel ...");
  } else {
    snprintf(s_status_text, sizeof(s_status_text), "Lade Panel ...\n%lu %%",
             (unsigned long)(received * 100 / total));
    show_status(s_status_text);
  }
}

static void on_done(GBitmap *bitmap, uint32_t png_bytes, uint32_t elapsed_ms) {
  gbitmap_destroy(s_bitmap);
  s_bitmap = bitmap;

  GRect window_bounds = layer_get_bounds(window_get_root_layer(s_window));
  GSize size = gbitmap_get_bounds(bitmap).size;
  layer_set_frame(bitmap_layer_get_layer(s_bitmap_layer), GRect(0, 0, size.w, size.h));
  bitmap_layer_set_bitmap(s_bitmap_layer, bitmap);
  scroll_layer_set_content_size(s_scroll_layer,
                                GSize(window_bounds.size.w, size.h));
  scroll_layer_set_content_offset(s_scroll_layer, GPointZero, false);

  snprintf(s_status_text, sizeof(s_status_text),
           "%dx%d px\n%lu Bytes in %lu ms\nHeap frei: %lu",
           size.w, size.h, (unsigned long)png_bytes, (unsigned long)elapsed_ms,
           (unsigned long)heap_bytes_free());
  APP_LOG(APP_LOG_LEVEL_INFO, "%s", s_status_text);
  show_status(s_status_text);
  s_hide_timer = app_timer_register(4000, hide_status, NULL);
}

static void on_failed(const char *reason) {
  snprintf(s_status_text, sizeof(s_status_text), "Fehler:\n%s", reason);
  show_status(s_status_text);
}

static void select_click(ClickRecognizerRef recognizer, void *context) {
  transfer_request(0);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_shadow_hidden(s_scroll_layer, true);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  scroll_layer_set_callbacks(s_scroll_layer, (ScrollLayerCallbacks){
    .click_config_provider = click_config_provider,
  });
  layer_add_child(root, scroll_layer_get_layer(s_scroll_layer));

  s_bitmap_layer = bitmap_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  scroll_layer_add_child(s_scroll_layer, bitmap_layer_get_layer(s_bitmap_layer));

  s_status_layer = text_layer_create(GRect(10, bounds.size.h - 80, bounds.size.w - 20, 70));
  text_layer_set_background_color(s_status_layer, GColorBlack);
  text_layer_set_text_color(s_status_layer, GColorWhite);
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_status_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_status_layer);
  bitmap_layer_destroy(s_bitmap_layer);
  scroll_layer_destroy(s_scroll_layer);
  gbitmap_destroy(s_bitmap);
  s_bitmap = NULL;
}

static void on_phone_ready(void) {
  transfer_request(0);
}

static void init(void) {
  transfer_init((TransferCallbacks){
    .progress = on_progress,
    .done = on_done,
    .failed = on_failed,
    .phone_ready = on_phone_ready,
  });

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  // Erst anfragen, wenn PebbleKit JS sich meldet; sonst geht die Anfrage verloren.
  show_status("Warte auf Handy ...");
}

static void deinit(void) {
  transfer_deinit();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
