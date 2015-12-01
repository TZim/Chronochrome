#include <pebble.h>

#include "chronochrome.h"
#include "util.h"
#include "appmsg.h"
#include "mode_info.h"

static unsigned appmsg_send_buffer_bytes = 1024;
static unsigned appmsg_recv_buffer_bytes = 1024;
  
static int appmsg_temperature = 999;
static int appmsg_sunrise = 0;
static int appmsg_sunset = 0;
static int appmsg_utc_offset = 99;
static int stock_data[2] = {KEY_STOCKS_NA, KEY_STOCKS_NA};
static bool appmsg_new_temperature = true;
static bool appmsg_new_sun = true;
static bool appmsg_new_utc_offset = true;
static bool appmsg_new_stock_data = true;
static bool appmsg_toggle = false;

bool get_appmsg_toggle (void) {
  return appmsg_toggle;
}

int get_temperature() {
  return appmsg_temperature;
}

int get_sunrise() {
  return appmsg_sunrise;
}

int get_sunset() {
  return appmsg_sunset;
}

int get_utc_offset() {
  return appmsg_utc_offset;
}

int get_stock_data(unsigned int n) {
  if (n >= sizeof(stock_data))
    return STOCK_NA;
  return stock_data[n];
}

bool new_temperature(){
  bool old = appmsg_new_temperature;
  appmsg_new_temperature = false;
  return old && (appmsg_temperature != 999);
}

bool new_sun() {
  bool old = appmsg_new_sun;
  appmsg_new_sun = false;
  return old;
}

bool new_utc_offset() {
  bool old = appmsg_new_utc_offset;
  appmsg_new_utc_offset = false;
  return old;
}

bool new_stock_data() {
  bool old = appmsg_new_stock_data;
  appmsg_new_stock_data = false;
  return old;
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  appmsg_toggle = !appmsg_toggle;
  // APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

// n: change factor * 1000.
static int stock_convert(int n) {
  if (n == STOCK_NA)
    return n;
  if (n > STOCK_NA - 1)
    n = STOCK_NA-1;
  else if (n < 1 - STOCK_NA)
    n = -STOCK_NA-1;
  return n;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "Inbox receive success!");
  
  // For all items
  while(t != NULL) {
    //APP_LOG(APP_LOG_LEVEL_INFO, "Key %d, value %d", (int)t->key, (int)t->value->int32);
    
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      appmsg_temperature = (int)t->value->int32;
      appmsg_new_temperature = true;
      break;
    case KEY_UTC_OFFSET:
      appmsg_utc_offset = (int)t->value->int32;
      appmsg_new_utc_offset = true;
      break;
    case KEY_DJIA:
      stock_data[0] = stock_convert((int)t->value->int32);
      if (stock_data[0] != STOCK_NA)
        appmsg_new_stock_data = true;
      break;
    case KEY_NASDAQ:
      stock_data[1] = stock_convert((int)t->value->int32);
      if (stock_data[1] != STOCK_NA)
        appmsg_new_stock_data = true;
      break;
    case KEY_SUNRISE:
      appmsg_sunrise = (int)t->value->int32;
      appmsg_new_sun = true;
      break;
    case KEY_SUNSET:
      appmsg_sunset = (int)t->value->int32;
      appmsg_new_sun = true;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
}

void init_appmsg_callbacks() {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  if (appmsg_recv_buffer_bytes > app_message_inbox_size_maximum())
    appmsg_recv_buffer_bytes = app_message_inbox_size_maximum();
  if (appmsg_send_buffer_bytes > app_message_outbox_size_maximum())
    appmsg_send_buffer_bytes = app_message_outbox_size_maximum();
  
  app_message_open(appmsg_recv_buffer_bytes, appmsg_send_buffer_bytes);
}

void deinit_appmsg_callbacks() {
  app_message_deregister_callbacks();
}

void ping_appmsg_info() {
  if (!BT_is_connected())
    return;
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "DictionaryIter not initialized");
  }
  else {
    dict_write_uint8(iter, 0, 0);
    app_message_outbox_send();
  }
}

