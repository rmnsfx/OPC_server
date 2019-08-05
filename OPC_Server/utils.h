#pragma once



timespec time_diff(timespec start, timespec end);
char* print_date_time(void);
void write_text_to_log_file(const char* text);
unsigned long long getTotalSystemMemory(void);
int getRam(void);
int getCurrentProccessMemory();