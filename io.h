#ifndef IO_H
#define IO_H
#include "model.h"

void clear_input_buffer(void);
int  prompt_int(const char* msg, int min, int max);
void prompt_string(const char* msg, char* out, int out_size);
int  prompt_yes_no(const char* msg);
/* 7주차: 실수 입력 */
float prompt_float(const char* msg, float min, float max);

int  save_csv(const char* path, const StudentStore* store);
int  load_csv(const char* path, StudentStore* store);

#endif
