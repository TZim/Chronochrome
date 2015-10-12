#pragma once

void init_stpw(void);
void deinit_stpw(void);

void stpw_set_mode(bool st_mode);
void stpw_handle_up_click(void);
void stpw_handle_down_click(void);

int persist_stpw_data(int index);
int restore_stpw_data(int index);
