#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>   /* size_t 사용을 위해 */

/* 학생 1명의 최대 이름 길이, 최대 과목 수 */
#define MAX_NAME_LEN   50
#define MAX_SUBJECTS   5

/* 학번 범위(원하면 바꿔도 됨) */
#define MIN_ID         1
#define MAX_ID         999999999

/* 학생 1명을 나타내는 구조체 */
typedef struct {
    int   id;                            /* 학번 */
    char  name[MAX_NAME_LEN];           /* 이름 (C 문자열) */
    int   subject_count;                /* 이번 학생이 들은 과목 수 (1~5) */
    int   scores[MAX_SUBJECTS];         /* 각 과목 점수 */
    float average;                      /* 평균 점수 */
    int   rank;                         /* 등수(1등, 2등, …) */
} Student;

/* 학생들을 담는 “저장소” 구조체 */
typedef struct {
    Student* data;    /* 힙에 할당된 Student 배열의 시작 주소 */
    size_t   size;    /* 현재 들어있는 학생 수 */
    size_t   capacity;/* 배열이 담을 수 있는 최대 학생 수(칸 수) */
} StudentStore;

/* 과목별 통계를 담기 위한 구조체 */
typedef struct {
    int   count;      /* 이 과목을 수강한 학생 수 */
    int   min;        /* 최소 점수 */
    int   max;        /* 최대 점수 */
    float average;    /* 평균 점수 */
} SubjectStats;

/* === 저장소 관련 === */
void init_store(StudentStore* s);
void free_store(StudentStore* s);

/* === 점수/평균/등급 === */
float      compute_average(const int* scores, int subject_count);
const char* grade_of_average(float avg);

/* === 학생 추가/조회/수정/삭제 === */
int       add_student(StudentStore* s, int id, const char* name,
    const int* scores, int subject_count);
Student* find_student_by_id(StudentStore* s, int id);
Student* find_student_by_name(StudentStore* s, const char* name);
int       update_student(StudentStore* s, int id,
    const int* new_scores, int subject_count);
int       update_student_name(StudentStore* s, int id, const char* new_name);
int       delete_student(StudentStore* s, int id);

/* === 정렬 관련 === */
void sort_by_average(StudentStore* s);
void sort_by_name(StudentStore* s);
void sort_by_average_then_name(StudentStore* s);
void sort_by_name_then_id(StudentStore* s);
void sort_by_id(StudentStore* s);

/* === 등수/통계 === */
void  recompute_ranks(const StudentStore* s);
void  compute_subject_stats(const StudentStore* s,
    SubjectStats out_stats[MAX_SUBJECTS]);

/* === 이름 부분 문자열 검색(대소문자 무시) === */
size_t find_students_by_name_substr_ci(StudentStore* s,
    const char* needle,
    Student** out_array, size_t max_out);

/* === 보기용 도우미 === */
void print_student_pretty(const Student* st);

/* === 6주차 기능(텍스트 리포트, CSV 머지) === */
int export_report_txt(const char* path, const StudentStore* store);

int import_merge_csv(const char* path, StudentStore* store,
    int overwrite_existing,
    int* out_added, int* out_updated, int* out_skipped);

#endif /* MODEL_H */

