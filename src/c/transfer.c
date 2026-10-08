#include "transfer.h"

static TransferCallbacks s_cb;
static uint32_t s_inbox_size;
static uint8_t *s_buffer;
static uint32_t s_total;
static uint32_t s_received;
static time_t s_start_s;
static uint16_t s_start_ms;

static uint32_t elapsed_ms(void) {
  time_t s;
  uint16_t ms;
  time_ms(&s, &ms);
  return (uint32_t)(s - s_start_s) * 1000 + ms - s_start_ms;
}

static void reset_buffer(void) {
  free(s_buffer);
  s_buffer = NULL;
  s_total = 0;
  s_received = 0;
}

static void fail(const char *reason) {
  reset_buffer();
  if (s_cb.failed) s_cb.failed(reason);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  if (dict_find(iter, MESSAGE_KEY_READY)) {
    if (s_cb.phone_ready) s_cb.phone_ready();
    return;
  }

  Tuple *error = dict_find(iter, MESSAGE_KEY_ERROR);
  if (error) {
    fail(error->value->cstring);
    return;
  }

  Tuple *total = dict_find(iter, MESSAGE_KEY_TOTAL_SIZE);
  Tuple *offset = dict_find(iter, MESSAGE_KEY_OFFSET);
  Tuple *data = dict_find(iter, MESSAGE_KEY_DATA);
  if (!total || !offset || !data) return;

  uint32_t off = offset->value->uint32;
  if (off == 0) {
    reset_buffer();
    s_total = total->value->uint32;
    s_buffer = malloc(s_total);
    if (!s_buffer) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "kein Speicher fuer %lu Bytes PNG", (unsigned long)s_total);
      fail("Zu wenig Speicher");
      return;
    }
  }
  if (!s_buffer || off != s_received || off + data->length > s_total) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "unerwartetes Stueck bei %lu", (unsigned long)off);
    return;
  }

  memcpy(s_buffer + off, data->value->data, data->length);
  s_received += data->length;
  if (s_cb.progress) s_cb.progress(s_received, s_total);

  if (s_received == s_total) {
    uint32_t png_bytes = s_total;
    GBitmap *bitmap = gbitmap_create_from_png_data(s_buffer, s_total);
    reset_buffer();
    if (!bitmap) {
      fail("PNG nicht lesbar");
      return;
    }
    if (s_cb.done) s_cb.done(bitmap, png_bytes, elapsed_ms());
  }
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Nachricht verworfen: %d", (int)reason);
}

static void outbox_failed(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  fail("Handy nicht erreichbar");
}

void transfer_init(TransferCallbacks callbacks) {
  s_cb = callbacks;
  s_inbox_size = app_message_inbox_size_maximum();
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_register_outbox_failed(outbox_failed);
  app_message_open(s_inbox_size, 64);
  APP_LOG(APP_LOG_LEVEL_INFO, "Inbox %lu Bytes", (unsigned long)s_inbox_size);
}

void transfer_deinit(void) {
  app_message_deregister_callbacks();
  reset_buffer();
}

void transfer_request(int32_t panel_id) {
  reset_buffer();
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    fail("Outbox belegt");
    return;
  }
  dict_write_int32(iter, MESSAGE_KEY_REQUEST, panel_id);
  dict_write_int32(iter, MESSAGE_KEY_INBOX_SIZE, (int32_t)s_inbox_size);
  time_ms(&s_start_s, &s_start_ms);
  if (s_cb.progress) s_cb.progress(0, 0);
  app_message_outbox_send();
}
