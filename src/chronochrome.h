#pragma once
  
#include "util.h"
  
void text_layer_setup(TextLayer *txlayer, char *buffer,	GFont font, GColor color, GTextAlignment align);
TextLayer *text_layer_new(Layer *parent_layer, GRect bounds, char *buffer,
	                      GFont font, GColor color, GTextAlignment align);
bool BT_is_connected(void);

typedef enum { status_void, status_reset, status_running, status_paused, status_init, status_done }
              status_state_t;
void update_stpw_status(status_state_t stat);
void update_ctdn_status(status_state_t stat);
void ping_for_info(time_t now);

void update_stpw_status_icon(status_state_t stat);
void update_ctdn_status_icon(status_state_t stat);

typedef enum { info_display, info_stocks, info_12v24, info_temp, info_usr1, info_usr2 } info_state_t;

info_state_t get_info_state(void);