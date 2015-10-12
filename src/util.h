#pragma once
  
#define CONST(name, val) enum { name = val }
#define RECT(x, y, w, h) (GRect){ .origin = {x, y}, .size = {w, h}}

#define myGreen GColorIslamicGreen
#define myRed GColorRed
#define myBlue GColorBlue
#define myYellow GColorRajah
#define myGray GColorDarkGray
  
int persist_read_data_if_exists(const uint32_t key, void *buffer, const size_t buffer_size);

typedef struct sec_ms {
	time_t sec;
	uint16_t ms;
} Sec_ms;

void get_current_sec_ms(Sec_ms *secms);
void add_sec_ms(Sec_ms *secms, Sec_ms inc);
void subtract_sec_ms(Sec_ms *secms, Sec_ms dec);
uint16_t hour_from_seconds(uint16_t secs);
uint16_t minute_from_seconds(uint16_t secs);
uint16_t dminute_from_seconds(uint16_t secs);
uint16_t second_from_seconds(uint16_t secs);
bool sec_ms_is_zero(Sec_ms n);

void localtm(time_t *time, struct tm *tm1);

#define FAKE_TIME false
#define FAKE_HOUR 9
#define FAKE_MIN 18
#define FAKE_SEC 00
#define FAKE_MDAY 23
#define FAKE_MON 7
#define FAKE_YEAR 2015
#define FAKE_DJIA 3
#define FAKE_NASDAQ 8
  
#define FAKE_LOC false // see also appmsg.js
