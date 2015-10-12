#include <pebble.h>
#include "util.h"
#include "main_ui.h"
#include "chronochrome.h"
#include "appmsg.h"
#include "mode_info.h"
  
static bool info_mode = true;

#define BLANK_STOCK "--.-"
static char djia_buffer[] = BLANK_STOCK;
static char nasdaq_buffer[] = BLANK_STOCK;

static int stock_djia = STOCK_NA; // persist
static int stock_nasdaq = STOCK_NA; // persist

TextLayer *djia_txlayer;
TextLayer *nasdaq_txlayer;

static void update_info_aux(int info, char *buffer, int size, TextLayer *txlayer) {
  if (info < 0) {
    info = -info;
  }
  if (info >= STOCK_NA)
    snprintf(buffer, size, BLANK_STOCK);
  else                
    snprintf(buffer, size, "%02u.%01u", info / 10, info % 10);
}

static int last_stock_djia = STOCK_NA + 1;
static int last_stock_nasdaq = STOCK_NA + 1;

static void refresh_info(void);

void update_info(void) {
        
  if (stock_djia != last_stock_djia) {
    last_stock_djia = stock_djia;
    if (FAKE_TIME)
      stock_djia = FAKE_DJIA;
    update_info_aux(stock_djia, djia_buffer,sizeof(djia_buffer), djia_txlayer);
  }
  
  if (stock_nasdaq != last_stock_nasdaq) {
    last_stock_nasdaq = stock_nasdaq;
    if (FAKE_TIME)
      stock_nasdaq = FAKE_NASDAQ;
    update_info_aux(stock_nasdaq, nasdaq_buffer,sizeof(nasdaq_buffer), nasdaq_txlayer);
  }
  
  if (info_mode) {
    refresh_info();
  }
}

static void reset_info(void) {
  stock_djia = stock_nasdaq = STOCK_NA;
  last_stock_djia = last_stock_nasdaq = -1;
  snprintf(djia_buffer, sizeof(djia_buffer), BLANK_STOCK);
  snprintf(nasdaq_buffer, sizeof(nasdaq_buffer), BLANK_STOCK);  
}

void update_stock_data(void) {
  if (new_stock_data()) {
    int datum = get_stock_data(0);
    if (datum != STOCK_NA)
      stock_djia = datum;
    
    datum = get_stock_data(1);
    if (datum != STOCK_NA)
      stock_nasdaq = datum;
    
    update_info();
  }
}

// Interface

int persist_info_data(int index) {
  persist_write_data(index++, &stock_djia, sizeof(stock_djia));
  persist_write_data(index++, &stock_nasdaq, sizeof(stock_nasdaq));

	return index;
}

int restore_info_data(int index) {
  persist_read_data_if_exists(index++, &stock_djia, sizeof(stock_djia));
  persist_read_data_if_exists(index++, &stock_nasdaq, sizeof(stock_nasdaq));
	return index;
}

void info_handle_up_click(void) {
  refresh_info();
}

void info_handle_down_click(void) {
  reset_info();
  refresh_info();
}

static void refresh_info() {
  
  bool info = get_info_state() == info_stocks;
  
  GColor txcol = stock_djia < 0 ? myRed : (stock_djia >= STOCK_NA ? GColorBlack : myGreen);  
  text_layer_set_text_color(djia_txlayer, info ? myYellow : txcol);
  text_layer_set_text(djia_txlayer, djia_buffer);
  
  txcol = stock_nasdaq < 0 ? myRed : (stock_nasdaq >= STOCK_NA ? GColorBlack : myGreen);
  text_layer_set_text_color(nasdaq_txlayer, info ? myYellow : txcol);
  text_layer_set_text(nasdaq_txlayer, nasdaq_buffer);
}

static void enable_info(void) {
  refresh_info();
}

void info_set_mode(bool inf_mode) {
	info_mode = inf_mode;
  if (inf_mode) {
    enable_info();
    update_info();
  }
}

void init_info(void) {
  djia_txlayer = get_infol_txlayer();
  nasdaq_txlayer = get_infor_txlayer();
}

void deinit_info(void) {
  djia_txlayer = nasdaq_txlayer = NULL;
}
