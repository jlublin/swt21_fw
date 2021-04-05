#pragma once

#define printf(...) print_str(__VA_ARGS__)
void print_str(const char *format, ...);
int alloc_tx_slot(uint16_t period, uint16_t bytes);
void free_tx_slot(int tx_handle);
void listener_init();
void listener_thread(void *parameters);
