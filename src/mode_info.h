#pragma once

void update_stock_data(void);
void update_info(void);
void info_set_mode(bool tm_mode);
void info_handle_up_click(void);
void info_handle_down_click(void);
void init_info(void);
void deinit_info(void);
int persist_info_data(int index);
int restore_info_data(int index);

#define STOCK_NA 1000
