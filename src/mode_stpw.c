#include <pebble.h>

#include "chronochrome.h"
#include "util.h"
#include "main_ui.h"

static bool stpw_mode = false;

typedef enum { stpw_stopped, stpw_running } stpw_state_t;
static stpw_state_t stpw_state = stpw_stopped; // persist

static TextLayer *stpw_txlayer;
static TextLayer *sthr_txlayer;
static TextLayer *stfr_txlayer;

static char stpw_buffer[] = "00:00";
static char sthr_buffer[] = "00h";
static char stfr_buffer[] = ".99s";

static AppTimer *stpw_timer = NULL;

static Sec_ms stpw_base_time = { .sec = 0, .ms = 0 }; // persist. valid only when stpw running
// Shows effective time since stpw started (revised after pause/resume).

static Sec_ms stpw_elps_time = { .sec = 0, .ms = 0 }; // persist. valid only when stpw paused
// Shows cumulative elapsed time (excludes paused time).

static bool stpw_maxed = false; // persist.

static void update_stpw(void);
static void handle_stpw_timer(void *ignore) {
	stpw_timer = NULL;
	update_stpw();
}

// Update, start, stop

// Called at load time and from button handlers (start, stop, mode) and timer handler
static void update_stpw(void) {
	if (stpw_timer != NULL) {
		app_timer_cancel(stpw_timer);
		stpw_timer = NULL;
	}
	if (!stpw_mode) {
    return;
  }

	uint32_t next;
	Sec_ms now;
	Sec_ms delta = { .sec = 0, .ms = 0 };

	switch (stpw_state) {
	case stpw_running:
		get_current_sec_ms(&now);
		delta = now;
		subtract_sec_ms(&delta, stpw_base_time);

		snprintf(stfr_buffer, sizeof(stfr_buffer), ".--s");

		// Sched handler at next elps second for updated display
		next = (uint32_t)stpw_base_time.ms;
		if (next < now.ms)
			next += 1000;
		next -= now.ms;
		stpw_timer = app_timer_register(next, handle_stpw_timer, NULL);
    update_stpw_status_icon(status_running);
		break;
    
	case stpw_stopped:
		delta = stpw_elps_time;
    if (stpw_maxed)
      delta.ms = 999;
      
		snprintf(stfr_buffer, sizeof(stfr_buffer), ".%02ds", delta.ms / 10);
    update_stpw_status_icon(sec_ms_is_zero(stpw_elps_time) ? status_reset : status_paused);
		break;
	}

	uint16_t display_hour = hour_from_seconds(delta.sec);
	uint16_t display_minute = minute_from_seconds(delta.sec);
	uint16_t display_second = second_from_seconds(delta.sec);
  
  if (stpw_maxed || display_hour > 99) {
    stpw_maxed = true;
    display_hour = 99;
    display_minute = 59;
    display_second = 59;
    stpw_state = stpw_stopped;
    update_stpw_status_icon(status_paused);
  }

	snprintf(stpw_buffer, sizeof(stpw_buffer), "%02d:%02d", display_minute, display_second);
	layer_mark_dirty(text_layer_get_layer(stpw_txlayer));

	snprintf(sthr_buffer, sizeof(sthr_buffer), "%02dh", display_hour);
	layer_mark_dirty(text_layer_get_layer(sthr_txlayer));

	layer_mark_dirty(text_layer_get_layer(stfr_txlayer));
}

// Click handlers

static void stpw_start() {
	switch (stpw_state) {
	case stpw_running: break; // just keep running
	case stpw_stopped:
		stpw_state = stpw_running; // start or resume

		get_current_sec_ms(&stpw_base_time);
		subtract_sec_ms(&stpw_base_time, stpw_elps_time);

		if (sec_ms_is_zero(stpw_elps_time))
			vibes_short_pulse();
		break;
	}
	update_stpw();
}

static void stpw_stop() {
	switch (stpw_state) {
	case stpw_stopped:
		stpw_elps_time = (Sec_ms) { 0, 0 }; // Reset
    stpw_maxed = false;
		break;
	case stpw_running: // Pause
		stpw_state = stpw_stopped;

		get_current_sec_ms(&stpw_elps_time);
		subtract_sec_ms(&stpw_elps_time, stpw_base_time);

		break;
	}
	update_stpw();
}

// Interface

static void enable_stpw(void);

void stpw_set_mode(bool st_mode) {
	stpw_mode = st_mode;
	if (st_mode) {
    enable_stpw();
		update_stpw();
  }
}

void stpw_handle_up_click(void) {
	stpw_stop();
}
void stpw_handle_down_click(void) {
	stpw_start();
}

int persist_stpw_data(int index) {
	persist_write_data(index++, &stpw_state, sizeof(stpw_state));
	persist_write_data(index++, &stpw_base_time, sizeof(stpw_base_time));
	persist_write_data(index++, &stpw_elps_time, sizeof(stpw_elps_time));
  persist_write_data(index++, &stpw_maxed, sizeof(stpw_maxed));
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "stpw_base_time= %d", (int)stpw_base_time.sec); //xx

	return index;
}

int restore_stpw_data(int index) {

	persist_read_data_if_exists(index++, &stpw_state, sizeof(stpw_state));
	persist_read_data_if_exists(index++, &stpw_base_time, sizeof(stpw_base_time));
	persist_read_data_if_exists(index++, &stpw_elps_time, sizeof(stpw_elps_time));
  persist_read_data_if_exists(index++, &stpw_maxed, sizeof(stpw_maxed));
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "stpw_base_time= %d", (int)stpw_base_time.sec); //xx

	return index;
}


// Init and deinit

static void enable_stpw(void) {
  text_layer_set_text_color(stpw_txlayer, myBlue);
  //text_layer_set_background_color(stpw_txlayer, GColorClear);
  text_layer_set_text_color(sthr_txlayer, myBlue);
  //text_layer_set_background_color(sthr_txlayer, GColorClear);
  text_layer_set_text_color(stfr_txlayer, myBlue);
  //text_layer_set_background_color(stfr_txlayer, GColorClear);
  
  text_layer_set_text(stpw_txlayer, stpw_buffer);
  text_layer_set_text(sthr_txlayer, sthr_buffer);
  text_layer_set_text(stfr_txlayer, stfr_buffer);
}

void init_stpw(void) {
  stpw_txlayer = get_time_txlayer();
  sthr_txlayer = get_infol_txlayer();
  stfr_txlayer = get_infor_txlayer();
  
  layer_mark_dirty(get_stsw_layer());
}

void deinit_stpw(void) {
  stpw_txlayer = sthr_txlayer = stfr_txlayer = NULL;
	if (stpw_timer != NULL)
		app_timer_cancel(stpw_timer);
}
