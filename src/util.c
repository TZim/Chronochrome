#include <pebble.h>
#include <util.h>

int persist_read_data_if_exists(const uint32_t key, void *buffer, const size_t buffer_size) {
	int val = E_DOES_NOT_EXIST;
	if (persist_exists(key))
		val = persist_read_data(key, buffer, buffer_size);
	return val;
}

void get_current_sec_ms(Sec_ms *secms) {
	secms->ms = time_ms(&(secms->sec), NULL);
}

void add_sec_ms(Sec_ms *secms, Sec_ms inc) {
  secms->ms += inc.ms;
	secms->sec += inc.sec;
	if (secms->ms > 1000) {
		secms->ms -= 1000;
    secms->sec++;
	}
}

void subtract_sec_ms(Sec_ms *secms, Sec_ms dec) {
	if (dec.ms > secms->ms) {
		secms->ms += 1000;
		dec.sec++;
	}
	secms->ms -= dec.ms;
	secms->sec -= dec.sec;
}

uint16_t hour_from_seconds(uint16_t secs) {
	return (secs / 3600) % 100;
}
uint16_t minute_from_seconds(uint16_t secs) {
	return (secs / 60) % 60;
}
uint16_t dminute_from_seconds(uint16_t secs) {
	return (secs / 60) % 100;
}
uint16_t second_from_seconds(uint16_t secs) {
	return secs % 60;
}

bool sec_ms_is_zero(Sec_ms n) {
	return n.sec == 0 && n.ms == 0;
}


void localtm(time_t *time, struct tm *tm1) {
  struct tm *tm2 = localtime(time);
  tm1->tm_sec = tm2->tm_sec;
  tm1->tm_min = tm2->tm_min;
  tm1->tm_hour = tm2->tm_hour;
  tm1->tm_mday = tm2->tm_mday;
  tm1->tm_mon = tm2->tm_mon;
  tm1->tm_year = tm2->tm_year;
  tm1->tm_wday = tm2->tm_wday;
  tm1->tm_yday = tm2->tm_yday;
}