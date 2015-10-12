#pragma once
  
#include "util.h"

CONST(KEY_TEMPERATURE, 0);
CONST(KEY_UTC_OFFSET, 1);
CONST(KEY_DJIA, 2);
CONST(KEY_NASDAQ, 3);
CONST(KEY_SUNRISE, 4);
CONST(KEY_SUNSET, 5);

int get_temperature(void);
int get_sunrise(void);
int get_sunset(void);
int get_utc_offset(void);
int get_stock_data(unsigned int n);
bool get_appmsg_toggle(void);
bool new_temperature(void);
bool new_sun(void);
bool new_utc_offset(void);
bool new_stock_data(void);

//void inbox_received_callback(DictionaryIterator *iterator, void *context);
//void inbox_dropped_callback(AppMessageResult reason, void *context);
//void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
//void outbox_sent_callback(DictionaryIterator *iterator, void *context);

void init_appmsg_callbacks(void);
void deinit_appmsg_callbacks(void);
void ping_appmsg_info(void);

