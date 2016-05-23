#include <pebble.h>
#include "chronochrome.h"
#include "main_ui.h"
#include "mode_info.h"
#include "mode_stpw.h"
#include "mode_ctdn.h"
#include "appmsg.h"
#include "util.h"

static char usrinfo1[] = "Chronochrome by ";  // persist
static char usrinfo2[] = "Cambridge Apps  ";  // persist

static char time_buffer[] = "23:59";
static char date_buffer[] = "2014-12-31 Sat...";
static const int date_size = sizeof("2015-12-31 Sa");
static char utc_buffer[] = "--z";
static int time_display_hour = 99;
static int time_display_minute = 99;

static char sunrise_buffer[] = "00:00";
static char sunset_buffer[] = "00:00";
static char temperature_buffer[] = "--C";
static bool temperature_F = false; // persist

static TextLayer *time_txlayer;

static bool time_24hr = true; // persist
static int utc_offset = 99; //24 - 4; // persist
static int temperature = 999; // persist
static time_t sunrise = 0; // persist
static time_t sunset = 0; // persist

static bool BT_connected = false;
static bool BT_changed = true;

static time_t last_ping_appmsg_time = 0;
CONST(ping_appmsg_seconds, 60 * 20);

void ping_for_info(time_t now); // forward references
static void update_wx(void);
static void update_sun(void);
static void update_appmsg_info(struct tm *tm);

typedef enum { mode_info, mode_stpw, mode_ctdn } wmode_t;
typedef enum { stpw_stopped, stpw_running } stpw_state_t;
typedef enum { ctdn_mh, ctdn_ml, ctdn_sh, ctdn_sl, ctdn_stopped, ctdn_running } ctdn_state_t;

static info_state_t info_state = info_display; // ephemeral, don't persist

info_state_t get_info_state (void) { return info_state; }

static wmode_t mode = mode_info; // persist
static status_state_t stpw_status = status_void; // persist
static status_state_t ctdn_status = status_void; // persist

static int fake_sec_inc = 0;
static int fake_min_inc = 0;

// Clock drawing

//static const int hr_offset = 2;
//static const int min_offset = 48 + 4;
//static const int sec_offset = 48 * 2 + 6;

static int clock_last_time = -1; // hr*3600 + min*60 + sec

static int clock_hr_tick;
static int clock_min_tick;
static int clock_sec_tick;

static Layer *block_layers[18]; // hours, minutes, seconds
//static GColor block_color[5];

typedef struct {
  int index;
} BlockData;

/*
static void draw_grid(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, myRed);
  graphics_fill_rect(ctx, RECT(3, 1, 6*6+3, 2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, RECT(3+6*6+3+10, 1, 6*6+4, 2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, myRed);
  graphics_fill_rect(ctx, RECT(3+2*(6*6+3+10)+1, 1, 6*6+4, 2), 0, GCornerNone);
} 
*/

static const int blockinc = 2;
static const int border_padx = 0;
static const int border_pady = 0;
static const int blockx = 6;
static const int blockdivs = 5;
static const int hrblockdivs = 3;
static int blocky;
static int hrblocky;
static const int dotoffx = 2;
//static const int dotoffy = 4;
static const int dotx = 2;
static const int doty = 2;
static const int block_gap = 1;
static const int block3_gap = 4;
static const int hr_min_gap = 4;
static const int min_sec_gap = 4;

static void draw_block_layer(Layer *layer, GContext *ctx, int divs, int tick, int auxtick) {
  
  BlockData *data = (BlockData *)layer_get_data(layer);
  int index = data->index;
  int blockn = index % 6;
  GColor dark = myGray;
  GColor light = GColorWhite;
  if (index >= 6)
    dark = index >= 12 ? myRed : GColorBlack;
  else if (time_display_hour >= 12)
    dark = GColorBlack;
  
  bool ascending = tick < 6 * divs;
  
  if (!ascending)
    tick -= 6 * divs;
  
  int targn = tick / divs;
  int subtick = tick % divs;
  
  bool blockdark = ascending == (blockn <= targn);
  bool dotdark = !blockdark;
  
  int height = divs * blockinc;
  int ht = height;
  int dotoffy = divs - 1;
            
  if (targn == blockn) {
    ht = blockinc * subtick + auxtick;
    if (subtick < (divs+1) /2)
      dotdark = !dotdark;
  }
  //else return; //xxxx

  graphics_context_set_fill_color(ctx, blockdark ? dark : light);
  graphics_fill_rect(ctx, RECT(0, height - ht, blockx, ht), 0, GCornerNone);
  
  graphics_context_set_fill_color(ctx, blockdark ? light : dark);
  graphics_fill_rect(ctx, RECT(0, 0, blockx, height - ht), 0, GCornerNone);
  
  int dht = 0;
  if (targn == blockn && ht >= dotoffy && ht < dotoffy + doty)
    dht += auxtick;
  
  graphics_context_set_fill_color(ctx, dotdark ? light : dark);
  graphics_fill_rect(ctx, RECT(dotoffx, dotoffy + doty - dht, dotx, dht), 0, GCornerNone);
  
  graphics_context_set_fill_color(ctx, dotdark ? dark : light);
  graphics_fill_rect(ctx, RECT(dotoffx, dotoffy, dotx, doty - dht), 0, GCornerNone);
}

static void draw_block_hr_layer(Layer *layer, GContext *ctx) {
  int auxtick = clock_min_tick % (60 / hrblockdivs);
  auxtick = auxtick * blockinc / (60 / hrblockdivs);
  draw_block_layer(layer, ctx, hrblockdivs, clock_hr_tick, auxtick);
}
static void draw_block_min_layer(Layer *layer, GContext *ctx) {
  draw_block_layer(layer, ctx, blockdivs, clock_min_tick, 0);
}
static void draw_block_sec_layer(Layer *layer, GContext *ctx) {
  draw_block_layer(layer, ctx, blockdivs, clock_sec_tick, 0);
}

  
static void update_block(int tick, int divs, int index_offset) {

  tick = tick -1;  // Previous tick is the one to update (.e.g at 0000, 2359 changes)
  if (tick < 0)
    tick = 12 * divs - 1;
  
  int index = tick / divs;
  if (index >= 6)
    index -= 6;
      
  layer_mark_dirty(block_layers[index + index_offset]);  // Trigger re-draw
}

static bool full_hours = false;

// True=inc update. False=needs full update.
bool update_clock_incrementally(struct tm *tm) {
  
  int sec_tick = tm->tm_sec;
  int min_tick = tm->tm_min;
  int hr_tick = tm->tm_hour;
  
  int time = hr_tick * 3600 + min_tick * 60 + sec_tick;
  
  if (time == clock_last_time)
    return true;
  
  if (hr_tick >= 12)
    hr_tick -= 12;
  hr_tick *= hrblockdivs;
  if (!full_hours)
    hr_tick += min_tick / (60 / hrblockdivs);
  
  clock_sec_tick = sec_tick;
  clock_min_tick = min_tick;
  clock_hr_tick = hr_tick;
  
  int last_time = clock_last_time;
  clock_last_time = time;
  
  time = time - 1;
  if (time < 0)
    time = 23 * 3600 + 59 * 60 + 59;
  
  if (time != last_time)
    return false;
  
  update_block(sec_tick, blockdivs, 12);  
  if (sec_tick != 0)
    return true;
  
  update_block(min_tick, blockdivs, 6);
 
  update_block(hr_tick, hrblockdivs, 0);  
  
  return true;
}

static void load_block_layers(Layer *clayer) {
    
  blocky = blockdivs * blockinc;
  hrblocky = hrblockdivs * blockinc;
  
  LayerUpdateProc proc = draw_block_hr_layer;
  
  for (int i = 0, j = 0, i_offset = border_padx, j_offset = border_pady; i < 18; i++, j++, i_offset += blockx + block_gap) {
    if (i == 6) {
      j = 0;
      i_offset = border_padx;
      j_offset += hrblocky + hr_min_gap;
      proc = draw_block_min_layer;
    } 
    else if (i == 12) {
      j = 0;
      i_offset = border_padx;
      j_offset += blocky + min_sec_gap;
      proc = draw_block_sec_layer;
    }
    
    Layer *layer = layer_create_with_data(RECT(i_offset, j_offset, blockx, blocky), sizeof(BlockData));
    block_layers[i] = layer;
    layer_add_child(clayer, layer);
    BlockData *data = (BlockData *)layer_get_data(layer);
    data->index = i;
    
    layer_set_update_proc(layer, proc);
    
    if (j == 2)
      i_offset += block3_gap - block_gap;
  }
}

static void unload_block_layers(void) {
  for (int i = 0; i < 18; i++) {
    layer_destroy(block_layers[i]);    
    block_layers[i] = NULL;
  }
}


// Tick and timer handlers

static bool first_tick = true;

static void update_utc(struct tm *tm) {
  if (utc_offset == 99) {
    return;
  }
  int utc_hour = tm->tm_hour + utc_offset;
	if (utc_hour < 0)
		utc_hour += 24;
  else if (utc_hour >= 24)
    utc_hour -= 24;

	snprintf(utc_buffer, sizeof(utc_buffer), "%02dz", utc_hour);
	
	layer_mark_dirty(text_layer_get_layer(get_utc_txlayer()));
}

static void refresh_min(void) {
  
  if (time_24hr) {
    if (info_state == info_12v24)
      snprintf(time_buffer, sizeof(time_buffer), "24:%02d", time_display_minute);
    else
		  snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d", time_display_hour, time_display_minute);
  }
  else {
    int hr = time_display_hour;
    if (hr > 12)
      hr -= 12;
    if (hr == 0)
      hr = 12;
	  snprintf(time_buffer, sizeof(time_buffer), "%2d:%02d", hr, time_display_minute);
  }
  
  if (mode == mode_info) {
    GColor col = time_display_hour < 12 ? myGray : GColorBlack;
    text_layer_set_text_color(time_txlayer, info_state == info_12v24 ? myYellow : col);
    layer_mark_dirty(text_layer_get_layer(time_txlayer));
  }
  
  time_t now = time(NULL);
  if (last_ping_appmsg_time == 0)
    last_ping_appmsg_time = now; // Skip 1st ping bc 1st data spontaneous
  else if (now > last_ping_appmsg_time + ping_appmsg_seconds){
    if  (BT_is_connected()) {
      ping_for_info(now);
    } // else { APP_LOG(APP_LOG_LEVEL_INFO, "Ping skipped"); }
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "update_min %d %s", (int)now, time_buffer);
}

static void update_min(struct tm *tm) {
  time_display_hour = tm->tm_hour;
  time_display_minute = tm->tm_min;
  
  refresh_min();
}

static void refresh_time(void) {
  refresh_min();
}


static void update_sec(struct tm *tm) {
  update_appmsg_info(tm);
  
  if (!update_clock_incrementally(tm))
    layer_mark_dirty(get_clock_layer());
}

static void update_date(struct tm *tm) {
	strftime(date_buffer, sizeof(date_buffer), "%Y-%m-%d %a", tm);
	date_buffer[date_size-1] = '\0'; // 3-char -> 2-char weekday to fit
	layer_mark_dirty(text_layer_get_layer(get_date_txlayer()));
}

void time_handle_second_tick(struct tm *tm, TimeUnits units_changed) {
	if ((units_changed & MINUTE_UNIT) != 0) {
		update_min(tm);

		if ((units_changed & HOUR_UNIT) != 0) {
			if (mode == mode_info && !first_tick) {
        if (!FAKE_TIME)
				  vibes_double_pulse();
			}

			update_utc(tm);
			if ((units_changed & DAY_UNIT) != 0) {
				update_date(tm);
			}
		}
	}
	update_sec(tm);
  
	first_tick = false;
}

struct tm fake_time;

static void update_BT(void);
static void handle_second_tick(struct tm *tm, TimeUnits units_changed) {
  if (FAKE_TIME) {
    tm = &fake_time;
    tm->tm_hour = FAKE_HOUR;
    tm->tm_min = FAKE_MIN + fake_min_inc;
    tm->tm_sec = FAKE_SEC + fake_sec_inc;
    tm->tm_mday = FAKE_MDAY;
    tm->tm_wday = FAKE_WDAY;
    tm->tm_mon = FAKE_MON;
    tm->tm_year = FAKE_YEAR - 1900;
    units_changed |= 
      SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT | YEAR_UNIT;
  }
  time_handle_second_tick(tm, units_changed);    
  
  update_BT();
}


static void refresh_usrinfo_aux(TextLayer *txlyr, char *usrinfo, size_t n, bool info_this) {
  text_layer_set_text_color(txlyr, info_this ? myYellow : GColorBlack);
  // snprintf(usrinfo_buffer, n, "%s", usrinfo);
  layer_mark_dirty(text_layer_get_layer(txlyr));
}

static void refresh_usrinfo1(void) {
  refresh_usrinfo_aux(get_usrinfo1_txlayer(), usrinfo1, sizeof(usrinfo1), info_state == info_usr1);
}

static void refresh_usrinfo2(void) {
  refresh_usrinfo_aux(get_usrinfo2_txlayer(), usrinfo2, sizeof(usrinfo2), info_state == info_usr2);
}

static void update_usrinfo(void) {
  refresh_usrinfo1();
  refresh_usrinfo2();
}

// Status icons

static GPoint points_ltri[] = {{4,0}, {12,5}, {4, 10}};
static GPath *path_ltri = NULL;
static GRect rect_sqr = {{4, 1}, {8, 8}};
static GRect rect_lbar = {{4, 1}, {3, 8}};
static GRect rect_rbar = {{9, 1}, {3, 8}};
static GRect rect_tbar = {{4, 1}, {8, 3}};
static GRect rect_bbar = {{4, 6}, {8, 3}};
GPathInfo info_ltri = {3, points_ltri};

static void draw_status(GContext *ctx, status_state_t stat, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  switch(stat) {
    case status_void:
    case status_reset: graphics_fill_rect(ctx, rect_sqr, 0, 0); break;
    case status_done: graphics_context_set_fill_color(ctx, myRed); // (ctdn only) like running but red
    case status_running: gpath_draw_filled(ctx, path_ltri); break;
    case status_paused: graphics_fill_rect(ctx, rect_lbar, 0, 0);
                        graphics_fill_rect(ctx, rect_rbar, 0, 0); break;
    case status_init: graphics_fill_rect(ctx, rect_tbar, 0, 0);
                      graphics_fill_rect(ctx, rect_bbar, 0, 0); break; 
  }
}

static void draw_stpw_status(Layer *lyr, GContext *ctx) {
  draw_status(ctx, stpw_status, myBlue);
}

static void draw_ctdn_status(Layer *lyr, GContext *ctx) {
  draw_status(ctx, ctdn_status, myGreen);
}

static void set_stpw_status(status_state_t stat) {
  stpw_status = stat;
  layer_mark_dirty(get_stsw_layer());
}

static void set_ctdn_status(status_state_t stat) {
  ctdn_status = stat;
  layer_mark_dirty(get_stcd_layer());
}

void update_stpw_status_icon(status_state_t stat) {
  if (stat != stpw_status)
    set_stpw_status(stat);
}

void update_ctdn_status_icon(status_state_t stat) {
  if (stat != ctdn_status)
    set_ctdn_status(stat);
}


// Appmsg

static void update_appmsg_info(struct tm *tm) {
  int datum;

  if (new_utc_offset()) {
    datum = get_utc_offset();
    if (datum != 99)
      utc_offset = datum;
    update_utc(tm);
  }
  if (new_temperature()) {
    datum = get_temperature();
    if (FAKE_TIME)
      datum = FAKE_TEMP;
    if (datum != 99)
      temperature = datum;
    update_wx();
  }
  if (new_sun()) {
    datum = get_sunrise();
    if (datum != 0) {
      sunrise = (time_t)datum;
    }
    datum = get_sunset();
    if (datum != 0) {
      sunset = (time_t)datum;
    }
    update_sun();
  }
  update_stock_data();
}

void ping_for_info(time_t now) {
  ping_appmsg_info();
  last_ping_appmsg_time = now;
}

static int last_temperature = -1000;

static void refresh_temperature() {
  
  int temp = temperature;
  GColor col = myGreen;
  
  if (temperature == 999)
    return;
  
  if (!temperature_F) {
    if (temp > 99)
      temp = 99;
    else if (temp < -99)
      temp = -99;
    if (temp < 0) {
      temp = -temp;
      col = myRed;
    }
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%02dC", temp);
  } else {
    temp = temp * 9 / 5 + 32;
    if (temp > 999)
      temp = 999;
    else if (temp < -999)
      temp = -999;
    if (temp < 0) {
      temp = -temp;
      col = myRed;
    }
    if (temp > 99)
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%03d", temp);
    else
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%02dF", temp);
  }
  
  TextLayer *txlyr = get_temperature_txlayer();
  text_layer_set_text_color(txlyr, info_state == info_temp ? myYellow :col);
  layer_mark_dirty(text_layer_get_layer(txlyr));
}

static void update_temperature(void) {
  if (temperature == last_temperature)
    return;
  last_temperature = temperature;
  refresh_temperature();
}

static int unix_utc_to_local_hhmm(time_t time) {
  struct tm tm1;
  localtm(&time, &tm1);
  int hh = tm1.tm_hour;
  return (hh << 16) + tm1.tm_min;
}

static int last_sunrise = -1;
static int last_sunset = -1;

static void refresh_sun_aux(char *buffer, size_t size, int hhmm) {
  int hh = hhmm >> 16;
  int mm = hhmm - (hh << 16);
  
  if (time_24hr) 
    snprintf(buffer, size, "%02d:%02d", hh, mm);
  else {
    if (hh == 0)
      hh = 12;
    else if (hh > 12)
      hh -= 12;
    snprintf(buffer, size, "%2d:%02d", hh, mm);
  }
}

static void refresh_sun(void) {  
  refresh_sun_aux(sunrise_buffer, sizeof(sunrise_buffer), unix_utc_to_local_hhmm(sunrise));
  refresh_sun_aux(sunset_buffer, sizeof(sunset_buffer), unix_utc_to_local_hhmm(sunset));
  
  layer_mark_dirty(text_layer_get_layer(get_srise_txlayer()));
  layer_mark_dirty(text_layer_get_layer(get_sset_txlayer()));
}

static void update_sun(void) {
  if (sunrise == 0 || sunset == 0)
    return;
  if (sunrise == last_sunrise && sunset == last_sunset)
    return;
  last_sunrise = sunrise;
  last_sunset = sunset;
  refresh_sun();
}

static void update_wx(void) {
  update_temperature();
  update_sun();
}

//GPoint BTpoints[] = {{0, 7}, {8, 15}, {4, 19}, {4, 3}, {8, 7}, {0, 0}};
GPoint BTpoints[] = {{7, 6}, {15, 0}, {19, 3}, {3, 3}, {7, 0}, {15, 6}};

static void draw_BT(Layer *btlay, GContext *ctx) {
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, BT_connected ? myBlue : myRed);
  graphics_context_set_stroke_width(ctx, 1);
  int n = 6; // sizeof(BTpoints);
  for (int i = 0; i < n-1; i++) {
    graphics_draw_line(ctx, BTpoints[i], BTpoints[i+1]);
    graphics_draw_line(ctx, GPoint(BTpoints[i].x+1, BTpoints[i].y+0),
                            GPoint(BTpoints[i+1].x+1, BTpoints[i+1].y+0));
  }
}

static void update_BT(void) {
  if (BT_changed) {
    layer_mark_dirty(get_bt_layer());
    BT_changed = false;
    if (!BT_connected)
      vibes_long_pulse();
  }
}


// Battery/bluetooth state handlers

static int last_batt_level = -1;
static int last_batt_charging = -1;

static void draw_batt(Layer *battlay, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, myGreen);
  graphics_context_set_fill_color(ctx, last_batt_charging == 1 ? myRed : myGreen);
  int height = 2 * last_batt_level;
  //int pad = 20 - height;
  int xpad = 2;
  int ypad = 0;
  graphics_fill_rect(ctx, RECT(xpad, ypad, height+2, 5), 0, GCornerNone);
  graphics_draw_line(ctx, (GPoint){26, 1+ypad}, (GPoint){26, 3+ypad});
  graphics_draw_line(ctx, (GPoint){25, 1+ypad}, (GPoint){25, 3+ypad});
  graphics_draw_line(ctx, (GPoint){24, 0+ypad}, (GPoint){24, 4+ypad});
}

static void handle_battery_state(BatteryChargeState state) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "batt0 %d %d", last_batt_level, last_batt_charging);
  
	int level = state.charge_percent / 10;
	if (level > 10)
		level = 10;
  int charging = state.is_charging ? 1 : 0;
  
  if (level == last_batt_level && charging == last_batt_charging)
    return;
  last_batt_level = level;
  last_batt_charging = charging;
  
  // APP_LOG(APP_LOG_LEVEL_INFO, "batt %d %d", level, charging);
  
  layer_mark_dirty(get_batt_layer());
}

static void handle_bt_state(bool connected) {
  if (BT_connected != connected) {
    BT_changed = true;
    BT_connected = connected;
  }
}

bool BT_is_connected(void) {
  return BT_connected;
}


// Click handlers

static void config_set_mode(bool info_mode) {
  if (!info_mode)
    return;
  
  TextLayer *tmtxlyr = get_time_txlayer();
  text_layer_set_text(tmtxlyr, time_buffer);
  refresh_time();
}

static void set_mode(void) {
  config_set_mode(mode == mode_info);
	info_set_mode(mode == mode_info);
	stpw_set_mode(mode == mode_stpw);
	ctdn_set_mode(mode == mode_ctdn);
}


static bool info_handle_middle_click(void) {
  if (info_state == info_display)
    return true;

  info_state = info_display;
  refresh_time();
  info_handle_up_click();
  refresh_temperature();
  refresh_usrinfo1();
  refresh_usrinfo2();
  return false;
}

static void middle_click_handler(ClickRecognizerRef recognizer, void *context) {
	switch (mode) {
    case mode_info:
      if (info_handle_middle_click())
         mode = mode_stpw;
		  break;
	  case mode_stpw:
		  mode = mode_ctdn;
		  break;
	  case mode_ctdn:
      if (ctdn_handle_middle_click())
	      mode = mode_info;
		  break;
	};
  set_mode();
}

static int usrinfo_buffer_index = -1;
static char usrinfo_selection[] = "X abcdefghijklmnopqrstuvwxyz0123456789.+-_@#";
static int usrinfo_selection_index = -1;

// updates are primarily to un/highlight txlayers as info_state changes.
static void config_handle_up_click(void) {
  switch (info_state) {
    case info_display:
      info_state = info_12v24;
      refresh_time();
      break;
    case info_12v24:
      if (BT_is_connected()) {
        info_state = info_stocks;
        refresh_time();
        info_handle_up_click();
      }
      else {
        info_state = info_temp;
        refresh_time();
        refresh_temperature();
      }
      break;
    case info_stocks:
      info_state = info_temp;
      info_handle_up_click();
      refresh_temperature();
      break;
    case info_temp:
      info_state = info_usr1;
      usrinfo_buffer_index = -1;
      usrinfo_selection_index = -1;
      refresh_temperature();
      refresh_usrinfo1();
      break;
    
    case info_usr1:
      if (usrinfo_buffer_index < 0) { // no buffer char selected. advance to usrinfo2
        info_state = info_usr2;
        usrinfo_selection_index = -1;
        
        refresh_usrinfo1();
        refresh_usrinfo2();
      }
      else {
        usrinfo_buffer_index++;
        usrinfo_selection_index = -1;
        if (usrinfo_buffer_index == sizeof(usrinfo1)) {
          info_state = info_usr2;
          usrinfo_buffer_index = -1;
          usrinfo_selection_index = -1;
          
          refresh_usrinfo1();
          refresh_usrinfo2();
        }
      }
      break;
    
    case info_usr2:
      if (usrinfo_buffer_index < 0) {
        info_state = info_display;
        
        refresh_usrinfo2();
        //refresh_time();
      }
      else {
        usrinfo_buffer_index++;
        usrinfo_selection_index = -1;
        if (usrinfo_buffer_index == sizeof(usrinfo2)) {
          info_state = info_display;
          
          refresh_usrinfo2();
        }
      }
      break;
  }
}

static void select_usrinfo_char(char *usrinfo_buffer) {
  if (usrinfo_buffer_index < 0) {
    usrinfo_buffer_index = 0;  // select 1st char
    return;
  }
  
  if (usrinfo_selection_index < 0)
    usrinfo_selection_index = 0;
  
  char ch = usrinfo_buffer[usrinfo_buffer_index];
  if (usrinfo_selection_index == 0  &&  ch >= 'a' && ch <= 'z') {
      ch -= 'a' - 'A';
      usrinfo_buffer[usrinfo_buffer_index] = ch;
  }          
  else {
    if (usrinfo_selection_index == 0)
      usrinfo_selection_index = 1;
    if (usrinfo_buffer[usrinfo_buffer_index] == usrinfo_selection[usrinfo_selection_index])
      usrinfo_selection_index++;
    usrinfo_buffer[usrinfo_buffer_index] = usrinfo_selection[usrinfo_selection_index];
  }
  usrinfo_selection_index++;
  if (usrinfo_selection_index == sizeof(usrinfo_selection)-1)
    usrinfo_selection_index = 0;
}

static void config_handle_down_click(void) {
  switch (info_state) {
    case info_display:
      break;
    case info_stocks:
      info_state = info_display;
      info_handle_down_click();
      ping_for_info(time(NULL));  // Refresh utc/stock/wx info
      break;
    case info_12v24:
      info_state = info_display;
      time_24hr = !time_24hr;
      refresh_time();
      refresh_sun();
      break;
    case info_temp:
      info_state = info_display;
      temperature_F = !temperature_F;
      refresh_temperature();
      break;
    case info_usr1:
      select_usrinfo_char(usrinfo1);
      refresh_usrinfo1();
      break;
    case info_usr2:
      select_usrinfo_char(usrinfo2);
      refresh_usrinfo2();
      break;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (FAKE_TIME) {
    fake_min_inc = (fake_min_inc + 1) % 60;
    return;
  }
	switch (mode) {
	  case mode_info: config_handle_up_click(); break;
    case mode_stpw: stpw_handle_up_click(); break;
	  case mode_ctdn: ctdn_handle_up_click(); break;
	}
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (FAKE_TIME) {
    fake_sec_inc = (fake_sec_inc + 1) % 60;
    return;
  }
	switch (mode) {
	  case mode_info: config_handle_down_click(); break;
	  case mode_stpw: stpw_handle_down_click(); break;
	  case mode_ctdn: ctdn_handle_down_click(); break;
	}
}

static void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, middle_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}


// Persistence

//static int debug_persist = 259;

void persist_data() {
	int index = 1;

	persist_write_data(index++, &mode, sizeof(mode));
  persist_write_data(index++, &stpw_status, sizeof(stpw_status));
  persist_write_data(index++, &ctdn_status, sizeof(ctdn_status));
  persist_write_data(index++, &utc_offset, sizeof(utc_offset));
  persist_write_data(index++, &temperature, sizeof(temperature));
  persist_write_data(index++, &sunrise, sizeof(sunrise));
  persist_write_data(index++, &sunset, sizeof(sunset));
  persist_write_data(index++, &time_24hr, sizeof(time_24hr));
  persist_write_data(index++, &temperature_F, sizeof(temperature_F));
  persist_write_data(index++, &usrinfo1, sizeof(usrinfo1));
  persist_write_data(index++, &usrinfo2, sizeof(usrinfo2));
  // persist_write_data(index++, &info_state, sizeof(info_state));

	index = persist_info_data(index);
	index = persist_stpw_data(index);
	index = persist_ctdn_data(index);
  
  //persist_write_data(index++, &debug_persist, sizeof(debug_persist));
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "persist index=%d %d", index, debug_persist);
}

static void restore_data() {
	int index = 1;
  
  // defaults if not previously persisted
  time_24hr = clock_is_24h_style();

	persist_read_data_if_exists(index++, &mode, sizeof(mode));
  persist_read_data_if_exists(index++, &stpw_status, sizeof(stpw_status));
  persist_read_data_if_exists(index++, &ctdn_status, sizeof(ctdn_status));
  persist_read_data_if_exists(index++, &utc_offset, sizeof(utc_offset));
  persist_read_data_if_exists(index++, &temperature, sizeof(temperature));
  persist_read_data_if_exists(index++, &sunrise, sizeof(sunrise));
  persist_read_data_if_exists(index++, &sunset, sizeof(sunset));
  persist_read_data_if_exists(index++, &time_24hr, sizeof(time_24hr));
  persist_read_data_if_exists(index++, &temperature_F, sizeof(temperature_F));
  persist_read_data_if_exists(index++, &usrinfo1, sizeof(usrinfo1));
  persist_read_data_if_exists(index++, &usrinfo2, sizeof(usrinfo2));
  // persist_read_data_if_exists(index++, &info_state, sizeof(info_state));

	index = restore_info_data(index);
	index = restore_stpw_data(index);
	index = restore_ctdn_data(index);
  
  //persist_read_data_if_exists(index++, &debug_persist, sizeof(debug_persist));
 
  //APP_LOG(APP_LOG_LEVEL_INFO, "restored index=%d %d", index, debug_persist);
}

// Initialization

static void update_time(void) {
	time_t now = time(NULL);
	struct tm tm1;
  struct tm *tm = &tm1;
  localtm(&now, tm);
    
  update_appmsg_info(tm);
	update_utc(tm);
  update_sec(tm);
  update_min(tm);
  update_date(tm);
  update_usrinfo();
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "update_time %d %s", (int)now, time_buffer);
}

void init_time(void) {
  clock_last_time = -1;
  
  time_txlayer = get_time_txlayer();
  
  text_layer_setup(time_txlayer, time_buffer, get_fontlg(), GColorBlack, GTextAlignmentCenter);
  text_layer_setup(get_utc_txlayer(), utc_buffer, get_fontmed(), GColorBlack, GTextAlignmentLeft);
  text_layer_setup(get_date_txlayer(), date_buffer, get_fontmed(), GColorBlack, GTextAlignmentCenter);

  init_appmsg_callbacks();

  //block_color[0]=GColorWhite; block_color[1]=myGreen; block_color[2]=myYellow; block_color[3]=myRed;
  //block_color[4]=myGray;
  
  path_ltri = gpath_create(&info_ltri);
}

void deinit_time(void) {
  deinit_appmsg_callbacks();
  gpath_destroy(path_ltri);
}
  
void text_layer_setup(TextLayer *txlayer, char *buffer,	GFont font, GColor color, GTextAlignment align) {
	text_layer_set_text(txlayer, buffer);
	text_layer_set_background_color(txlayer, GColorClear);
	text_layer_set_text_color(txlayer, color);
	text_layer_set_font(txlayer, font);
	text_layer_set_text_alignment(txlayer, align);
}

TextLayer *text_layer_new(Layer *parent_layer, GRect bounds, char *buffer,
	GFont font, GColor color, GTextAlignment align) {

	TextLayer *txlayer = text_layer_create(bounds);
	text_layer_setup(txlayer, buffer, font, color, align);

	Layer *lyr = text_layer_get_layer(txlayer);

	layer_add_child(parent_layer, lyr);

	return txlayer;
}

static void init(void) {    
  restore_data();
  
  show_main_ui();
    
  text_layer_setup(get_usrinfo1_txlayer(), usrinfo1, get_fontsm(), GColorBlack, GTextAlignmentLeft);
  text_layer_setup(get_usrinfo2_txlayer(), usrinfo2, get_fontsm(), GColorBlack, GTextAlignmentLeft);
  text_layer_setup(get_srise_txlayer(), sunrise_buffer, get_fontmed(), myGray, GTextAlignmentLeft);
  text_layer_setup(get_sset_txlayer(), sunset_buffer, get_fontmed(), GColorBlack, GTextAlignmentLeft);
  text_layer_setup(get_temperature_txlayer(), temperature_buffer, get_fontmed(), myGreen, GTextAlignmentLeft);
  
  //layer_set_update_proc(window_get_root_layer(win), update_clock_layer); //xxxx
  //layer_set_update_proc(get_clock_layer(), update_clock_layer); //xxxx
  load_block_layers(get_clock_layer());  //(window_get_root_layer(win)); //xxxx
  
  init_time();
  init_info();
  init_stpw();
  init_ctdn();
  window_set_click_config_provider(get_window(), click_config_provider);
  
  set_mode();
    
  update_time();
  update_wx();
  update_info();
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  battery_state_service_subscribe(handle_battery_state);
	bluetooth_connection_service_subscribe(handle_bt_state);
  
  layer_set_update_proc(get_bt_layer(), draw_BT);
  layer_set_update_proc(get_batt_layer(), draw_batt);
  layer_set_update_proc(get_stsw_layer(), draw_stpw_status);
  layer_set_update_proc(get_stcd_layer(), draw_ctdn_status);

	handle_bt_state(bluetooth_connection_service_peek());
  handle_battery_state(battery_state_service_peek());
}

static void deinit(void) {
  persist_data();
  
  bluetooth_connection_service_unsubscribe();
	battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  
  unload_block_layers();
  
  deinit_time();
  deinit_info();
  deinit_stpw();
  deinit_ctdn();
  
  hide_main_ui();
}

int main(void) {
  init();
  
  app_event_loop();
  
  deinit();
}