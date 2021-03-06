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
unsigned hour_from_seconds(time_t secs);
unsigned minute_from_seconds(time_t secs);
unsigned dminute_from_seconds(time_t secs);
unsigned second_from_seconds(time_t secs);
bool sec_ms_is_zero(Sec_ms n);

void localtm(time_t *time, struct tm *tm1);

#define FAKE_TIME false
#define FAKE_HOUR 15
#define FAKE_MIN 30
#define FAKE_SEC 00
#define FAKE_MDAY 23
#define FAKE_WDAY 2
#define FAKE_MON 1
#define FAKE_YEAR 2016
#define FAKE_DJIA -133
#define FAKE_NASDAQ 82
#define FAKE_TEMP -2
  
#define FAKE_LOC false // see also appmsg.js
