#pragma once
  
void init_ctdn(void);
void deinit_ctdn(void);

void ctdn_set_mode(bool st_mode);
bool ctdn_handle_middle_click(void);
void ctdn_handle_up_click(void);
void ctdn_handle_down_click(void);

int persist_ctdn_data(int index);
int restore_ctdn_data(int index);
