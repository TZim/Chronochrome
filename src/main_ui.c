#include <pebble.h>
#include "main_ui.h"

// BEGIN AUTO-GENERATED UI CODE; DO NOT MODIFY
static Window *s_window;
static GFont s_res_roboto_bold_subset_49;
static GFont s_res_gothic_28_bold;
static GFont s_res_gothic_14;
static TextLayer *time_txlayer;
static TextLayer *utc_txlayer;
static TextLayer *date_txlayer;
static TextLayer *srise_txlayer;
static TextLayer *sset_txlayer;
static TextLayer *temperature_txlayer;
static TextLayer *usrinfo1_txlayer;
static TextLayer *usrinfo2_txlayer;
static TextLayer *infol_txlayer;
static TextLayer *infor_txlayer;
static Layer *bt_layer;
static Layer *batt_layer;
static Layer *stsw_layer;
static Layer *stcd_layer;
static Layer *clock_layer;

static void initialise_ui(void) {
  s_window = window_create();
  #ifndef PBL_SDK_3
    window_set_fullscreen(s_window, 0);
  #endif
  
  s_res_roboto_bold_subset_49 = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_res_gothic_28_bold = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_res_gothic_14 = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  // time_txlayer
  time_txlayer = text_layer_create(GRect(0, 31, 144, 50));
  text_layer_set_text(time_txlayer, "00:00");
  text_layer_set_text_alignment(time_txlayer, GTextAlignmentCenter);
  text_layer_set_font(time_txlayer, s_res_roboto_bold_subset_49);
  layer_add_child(window_get_root_layer(s_window), (Layer *)time_txlayer);
  
  // utc_txlayer
  utc_txlayer = text_layer_create(GRect(1, 83, 34, 30));
  text_layer_set_text(utc_txlayer, "00z");
  text_layer_set_font(utc_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)utc_txlayer);
  
  // date_txlayer
  date_txlayer = text_layer_create(GRect(0, 111, 144, 28));
  text_layer_set_text(date_txlayer, "2000-01-01 Sat");
  text_layer_set_text_alignment(date_txlayer, GTextAlignmentCenter);
  text_layer_set_font(date_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)date_txlayer);
  
  // srise_txlayer
  srise_txlayer = text_layer_create(GRect(1, 139, 50, 28));
  text_layer_set_text(srise_txlayer, "06:00");
  text_layer_set_font(srise_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)srise_txlayer);
  
  // sset_txlayer
  sset_txlayer = text_layer_create(GRect(55, 139, 50, 28));
  text_layer_set_text(sset_txlayer, "18:00");
  text_layer_set_font(sset_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)sset_txlayer);
  
  // temperature_txlayer
  temperature_txlayer = text_layer_create(GRect(109, 139, 35, 28));
  text_layer_set_text(temperature_txlayer, "00C");
  text_layer_set_font(temperature_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)temperature_txlayer);
  
  // usrinfo1_txlayer
  usrinfo1_txlayer = text_layer_create(GRect(0, 8, 102, 16));
  text_layer_set_text(usrinfo1_txlayer, "Gary L. Drescher");
  text_layer_set_font(usrinfo1_txlayer, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)usrinfo1_txlayer);
  
  // usrinfo2_txlayer
  usrinfo2_txlayer = text_layer_create(GRect(0, 22, 100, 16));
  text_layer_set_text(usrinfo2_txlayer, "gld@alum.mit.edu");
  text_layer_set_font(usrinfo2_txlayer, s_res_gothic_14);
  layer_add_child(window_get_root_layer(s_window), (Layer *)usrinfo2_txlayer);
  
  // infol_txlayer
  infol_txlayer = text_layer_create(GRect(39, 83, 50, 28));
  text_layer_set_text(infol_txlayer, "00.00");
  text_layer_set_font(infol_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)infol_txlayer);
  
  // infor_txlayer
  infor_txlayer = text_layer_create(GRect(93, 83, 51, 28));
  text_layer_set_text(infor_txlayer, "99.99");
  text_layer_set_text_alignment(infor_txlayer, GTextAlignmentRight);
  text_layer_set_font(infor_txlayer, s_res_gothic_28_bold);
  layer_add_child(window_get_root_layer(s_window), (Layer *)infor_txlayer);
  
  // bt_layer
  bt_layer = layer_create(GRect(67, 2, 22, 12));
  layer_add_child(window_get_root_layer(s_window), (Layer *)bt_layer);
  
  // batt_layer
  batt_layer = layer_create(GRect(0, 3, 26, 11));
  layer_add_child(window_get_root_layer(s_window), (Layer *)batt_layer);
  
  // stsw_layer
  stsw_layer = layer_create(GRect(35, 1, 16, 10));
  layer_add_child(window_get_root_layer(s_window), (Layer *)stsw_layer);
  
  // stcd_layer
  stcd_layer = layer_create(GRect(47, 1, 16, 10));
  layer_add_child(window_get_root_layer(s_window), (Layer *)stcd_layer);
  
  // clock_layer
  clock_layer = layer_create(GRect(96, 2, 44, 34));
  layer_add_child(window_get_root_layer(s_window), (Layer *)clock_layer);
}

static void destroy_ui(void) {
  window_destroy(s_window);
  text_layer_destroy(time_txlayer);
  text_layer_destroy(utc_txlayer);
  text_layer_destroy(date_txlayer);
  text_layer_destroy(srise_txlayer);
  text_layer_destroy(sset_txlayer);
  text_layer_destroy(temperature_txlayer);
  text_layer_destroy(usrinfo1_txlayer);
  text_layer_destroy(usrinfo2_txlayer);
  text_layer_destroy(infol_txlayer);
  text_layer_destroy(infor_txlayer);
  layer_destroy(bt_layer);
  layer_destroy(batt_layer);
  layer_destroy(stsw_layer);
  layer_destroy(stcd_layer);
  layer_destroy(clock_layer);
}
// END AUTO-GENERATED UI CODE

static void handle_window_unload(Window* window) {
  destroy_ui();
}

Window *show_main_ui(void) {
  initialise_ui();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .unload = handle_window_unload,
  });
  window_stack_push(s_window, true);
  return s_window;
}

void hide_main_ui(void) {
  window_stack_remove(s_window, true);
}


Window *get_window(void) {return s_window;}
Layer *get_window_layer(void) {return window_get_root_layer(s_window);}
GFont get_fontlg(void) {return s_res_roboto_bold_subset_49;}
GFont get_fontmed(void) {return s_res_gothic_28_bold;}
GFont get_fontsm(void) {return s_res_gothic_14;}
Layer *get_clock_layer(void) {return clock_layer;}
TextLayer *get_time_txlayer(void) {return time_txlayer;}
TextLayer *get_utc_txlayer(void) {return utc_txlayer;}
TextLayer *get_date_txlayer(void) {return date_txlayer;}
TextLayer *get_srise_txlayer(void) {return srise_txlayer;}
TextLayer *get_sset_txlayer(void) {return sset_txlayer;}
TextLayer *get_temperature_txlayer(void) {return temperature_txlayer;}
TextLayer *get_usrinfo1_txlayer(void) {return usrinfo1_txlayer;}
TextLayer *get_usrinfo2_txlayer(void) {return usrinfo2_txlayer;}
Layer *get_bt_layer(void) {return bt_layer;}
Layer *get_stsw_layer(void) {return stsw_layer;}
Layer *get_stcd_layer(void) {return stcd_layer;}
Layer *get_batt_layer(void) {return batt_layer;}
TextLayer *get_infol_txlayer(void) {return infol_txlayer;}
TextLayer *get_infor_txlayer(void) {return infor_txlayer;}
// Layer *get_grid_layer(void) {return grid_layer;}

