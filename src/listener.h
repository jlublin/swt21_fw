#pragma once

#define printf(...) print_str(__VA_ARGS__)
void print_str(const char *format, ...);
void listener_thread(void *parameters);
