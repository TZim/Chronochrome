#include <pebble.h>

#include "chronochrome.h"
#include "util.h"
#include "main_ui.h"

static bool ctdn_mode = false;

typedef enum { ctdn_mh, ctdn_ml, ctdn_sh, ctdn_sl, ctdn_stopped, ctdn_running } ctdn_state_t;
static ctdn_state_t ctdn_state = ctdn_stopped; // persist

static TextLayer *ctdn_txlayer;
static TextLayer *ctin_txlayer;
static TextLayer *ctnoop_txlayer;

static char ctdn_buffer[] = "03:00";
static char ctin_buffer[] = "03:00";
static char null_buffer[] = "";

static AppTimer *ctdn_timer = NULL;

static Sec_ms ctdn_base_time = { .sec = 0, .ms = 0 }; // persist. valid only when ctdn running
// In non-overtime, shows effective time since ctdn started (adjusted after pause/resume).
// In overtime, shows time when target reached (no adjustment because no pause).

static Sec_ms ctdn_elps_time = { .sec = 0, .ms = 0 }; // persist. valid only when ctdn paused
// In non-overtime, shows cumulative elapsed time since started (excludes time paused).
// In overtime, shows elapsed time since target (no exclusion because no pause).

static time_t ctdn_targ_secs = 3 * 60; // persist
static bool ctdn_overtime = false; // persist

// Update, start, stop

static void update_ctdn(void);
static void handle_ctdn_timer(void *ignore) {
	ctdn_timer = NULL;
	update_ctdn();
}

static void update_ctin(void) {
  GColor col = myYellow;
  switch (ctdn_state) {
	case ctdn_running: case ctdn_stopped:
    col = myGreen;
	  snprintf(ctin_buffer, sizeof(ctin_buffer), "%02d:%02d",
		  minute_from_seconds(ctdn_targ_secs),
		  second_from_seconds(ctdn_targ_secs));
    break;
  case ctdn_mh:
		snprintf(ctin_buffer, sizeof(ctin_buffer), "%d-:--", (uint16_t)((ctdn_targ_secs / (10 * 60)) % 10));
    break;
  case ctdn_ml:
		snprintf(ctin_buffer, sizeof(ctin_buffer), "-%d:--", (uint16_t)((ctdn_targ_secs / 60) % 10));
    break;
  case ctdn_sh:
		snprintf(ctin_buffer, sizeof(ctin_buffer), "--:%d-", (uint16_t)((ctdn_targ_secs / 10) % 6));
    break;
  case ctdn_sl:
		snprintf(ctin_buffer, sizeof(ctin_buffer), "--:-%d", (uint16_t)(ctdn_targ_secs % 10));
    break;
  }
  text_layer_set_text_color(ctin_txlayer, col);
	layer_mark_dirty(text_layer_get_layer(ctin_txlayer));
}

// Called at load time and from button handlers (start, stop, mode) and timer handler
static void update_ctdn(void) {
	// check for timer expired (if resuming persisted app)
	// vibrate if done even if not in ctdn mode

	if (ctdn_timer != NULL) {
		app_timer_cancel(ctdn_timer);
		ctdn_timer = NULL;
	}

	uint32_t next;
	int remaining = ctdn_targ_secs - ctdn_elps_time.sec;  // For the non-running states.
	Sec_ms now;
	Sec_ms delta = { .sec = 0, .ms = 0 };

	switch (ctdn_state) {
	case ctdn_running: // running state can be countdown or overtime
		get_current_sec_ms(&now);
    
    delta = now;
	  subtract_sec_ms(&delta, ctdn_base_time);

    if (!ctdn_overtime) {

		  remaining = ctdn_targ_secs - delta.sec;
		  if (remaining > 0) {
        update_ctdn_status_icon(status_running);
      }
		  else { 
        // Here is the transition from running to done (overtime).
        ctdn_overtime = true;
        remaining = -remaining;
        ctdn_base_time.sec += ctdn_targ_secs; // Base time is time when target was reached.
			  vibes_long_pulse();
        update_ctdn_status_icon(status_done);
      }
    }
    else { // Already in overtime
      remaining = delta.sec;
      if (remaining < 60 * 60) {
        update_ctdn_status_icon(status_done);
      }
      else { // Automatic reset at 60 minutes overtime
        ctdn_elps_time = (Sec_ms) { 0, 0 }; // Reset
        ctdn_overtime = false;
        ctdn_state = ctdn_stopped;
        remaining = ctdn_targ_secs;
        update_ctdn_status_icon(status_reset);
      }
    }
    
    // Sched handler at next second (from base time) for updated running/done(overtime) display.
    if (ctdn_state == ctdn_running) { // May have just auto-stopped
      next = (uint32_t)ctdn_base_time.ms;
	    if (next < now.ms)
			  next += 1000;
	    next -= now.ms;
	    ctdn_timer = app_timer_register(next, handle_ctdn_timer, NULL);
    }
    
		break;
    
	case ctdn_stopped:
  	break;
    
	case ctdn_mh: case ctdn_ml: case ctdn_sh: case ctdn_sl:
		update_ctin();
    update_ctdn_status_icon(status_init);
		break;
	}
  
  if (ctdn_state == ctdn_stopped) {
    if (remaining == ctdn_targ_secs)
      update_ctdn_status_icon(status_reset);
    else
      update_ctdn_status_icon(status_paused);
  }
  
	if (ctdn_mode) {
	  uint16_t display_minute = dminute_from_seconds(remaining);
	  uint16_t display_second = second_from_seconds(remaining);

	  snprintf(ctdn_buffer, sizeof(ctdn_buffer), "%02d:%02d", display_minute, display_second);
    text_layer_set_text_color(ctdn_txlayer, ctdn_overtime ? myRed : myGreen);
	  layer_mark_dirty(text_layer_get_layer(ctdn_txlayer));
  }
}

static void ctdn_start() {
	int n = 0;

	switch (ctdn_state) {
	case ctdn_running: break; // just keep running
	case ctdn_stopped:
		if (ctdn_elps_time.sec < ctdn_targ_secs) {
			ctdn_state = ctdn_running; // start or resume

			get_current_sec_ms(&ctdn_base_time);
			subtract_sec_ms(&ctdn_base_time, ctdn_elps_time);

			if (sec_ms_is_zero(ctdn_elps_time))
				vibes_short_pulse();
		}
		break;

	case ctdn_mh:
		ctdn_targ_secs = (ctdn_targ_secs + 10 * 60) % (100 * 60);
		break;
	case ctdn_ml:
		n = (ctdn_targ_secs / 60) % 10;
		if (n == 9)
			ctdn_targ_secs -= 9 * 60;
		else
			ctdn_targ_secs += 60;
		break;
	case ctdn_sh:
		n = (ctdn_targ_secs / 10) % 6;
		if (n == 5)
			ctdn_targ_secs -= 5 * 10;
		else
			ctdn_targ_secs += 10;
		break;
	case ctdn_sl:
		n = ctdn_targ_secs % 10;
		if (n == 9)
			ctdn_targ_secs -= 9;
		else
			ctdn_targ_secs++;
		break;
	}
	update_ctdn();
}

static void ctdn_stop() {
	switch (ctdn_state) {
	case ctdn_stopped:
		if (sec_ms_is_zero(ctdn_elps_time)) // Already reset
			ctdn_state = ctdn_mh;
		else {
			ctdn_elps_time = (Sec_ms) { 0, 0 }; // Reset
      ctdn_overtime = false;
    }
		break;
    
	case ctdn_running: // Pause
		ctdn_state = ctdn_stopped;

    if (!ctdn_overtime) {
		  get_current_sec_ms(&ctdn_elps_time);
		  subtract_sec_ms(&ctdn_elps_time, ctdn_base_time);
    }
    else {
      ctdn_elps_time = (Sec_ms) { 0, 0 }; // Reset
      ctdn_overtime = false;
    }

		break;

	case ctdn_mh: ctdn_state = ctdn_ml; break;
	case ctdn_ml: ctdn_state = ctdn_sh; break;
	case ctdn_sh: ctdn_state = ctdn_sl; break;
	case ctdn_sl:
		if (ctdn_targ_secs == 0)
			ctdn_state = ctdn_mh;
		else {
			ctdn_state = ctdn_stopped;
			update_ctin();
		}
		break;
	}
	update_ctdn();
}


// Interface

static void enable_ctdn(void);

void ctdn_set_mode(bool ct_mode) {
	ctdn_mode = ct_mode;
	if (ct_mode) {
    enable_ctdn();
		update_ctdn();
    update_ctin();
  }
}

bool ctdn_handle_middle_click(void) {
  if (ctdn_state == ctdn_stopped || ctdn_state == ctdn_running)
    return true;
  
  ctdn_state = ctdn_stopped;
  update_ctin();
  return false;
}

void ctdn_handle_up_click(void) {
	ctdn_stop();
}
void ctdn_handle_down_click(void) {
	ctdn_start();
}


int persist_ctdn_data(int index) {
	persist_write_data(index++, &ctdn_state, sizeof(ctdn_state));
	persist_write_data(index++, &ctdn_base_time, sizeof(ctdn_base_time));
	persist_write_data(index++, &ctdn_elps_time, sizeof(ctdn_elps_time));
	persist_write_data(index++, &ctdn_targ_secs, sizeof(ctdn_targ_secs));
  persist_write_data(index++, &ctdn_overtime, sizeof(ctdn_overtime));

	return index;
}

int restore_ctdn_data(int index) {
	persist_read_data_if_exists(index++, &ctdn_state, sizeof(ctdn_state));
	persist_read_data_if_exists(index++, &ctdn_base_time, sizeof(ctdn_base_time));
	persist_read_data_if_exists(index++, &ctdn_elps_time, sizeof(ctdn_elps_time));
	persist_read_data_if_exists(index++, &ctdn_targ_secs, sizeof(ctdn_targ_secs));
  persist_read_data_if_exists(index++, &ctdn_overtime, sizeof(ctdn_overtime));

	return index;
}



// Init and deinit

void enable_ctdn(void) {
  text_layer_set_text_color(ctdn_txlayer, myGreen);
  //text_layer_set_background_color(ctdn_txlayer, GColorClear);
  text_layer_set_text_color(ctin_txlayer, myGreen);
  //text_layer_set_background_color(ctnoop_txlayer, GColorClear);
  
  text_layer_set_text(ctdn_txlayer, ctdn_buffer);
  text_layer_set_text(ctin_txlayer, ctin_buffer);
  text_layer_set_text(get_infol_txlayer(), null_buffer);
}

void init_ctdn(void) {
  ctdn_txlayer = get_time_txlayer();
  ctin_txlayer = get_infor_txlayer();
  ctnoop_txlayer = get_infol_txlayer();
  
  layer_mark_dirty(get_stcd_layer());
}

void deinit_ctdn(void) {
	if (ctdn_timer != NULL)
		app_timer_cancel(ctdn_timer);
  ctdn_txlayer = ctin_txlayer = NULL;
}
