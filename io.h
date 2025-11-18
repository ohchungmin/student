#ifndef IO_H
#define IO_H

#include "model.h"   /* Student, StudentStore 사용을 위해 */

/* 콘솔 입력 유틸리티 */
void clear_input_buffer(void);
int  prompt_int(const char* msg, int min, int max);
void prompt_string(const char* msg, char* out, int out_size);
int  prompt_yes_no(const char* msg);

/* CSV 파일 입출력 */
int  save_csv(const char* path, const StudentStore* store);
int  load_csv(const char* path, StudentStore* store);

#endif /* IO_H */
