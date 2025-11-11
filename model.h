#ifndef MODEL_H
#define MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define MAX_NAME_LEN 50
#define MAX_SUBJECTS 5
#define MIN_ID 1
#define MAX_ID 999999999

    /* ===== 학생 한 명 ===== */
    typedef struct {
        int   id;
        char  name[MAX_NAME_LEN];
        int   scores[MAX_SUBJECTS];
        int   subject_count;
        float average;   /* 현재 규칙(가중치 적용 포함)으로 계산된 평균 */
        int   rank;
    } Student;

    /* ===== 저장소 ===== */
    typedef struct {
        Student* data;
        size_t   size;
        size_t   capacity;
    } StudentStore;

    /* ===== 7주차: 전역 설정(가중치/컷) ===== */
    typedef struct {
        float subject_weights[MAX_SUBJECTS];  /* 기본 1.0f */
        float pass_threshold;                 /* 기본 60.0f */
    } GradeConfig;

    /* ===== 과목별 통계 ===== */
    typedef struct {
        int   count;
        int   min;
        int   max;
        float average;
    } SubjectStats;

    /* 저장소 수명주기 */
    void  init_store(StudentStore* s);
    void  free_store(StudentStore* s);

    /* 7주차: 설정 초기화 */
    void  init_config(GradeConfig* cfg);

    /* 평균/등급 */
    float compute_average(const int* scores, int subject_count);
    float compute_weighted_average(const int* scores, int subject_count,
        const float* weights);         /* 7주차 */
    const char* grade_of_average(float avg);
    const char* pass_label(float avg, float pass_threshold);       /* 7주차 */

    /* CRUD */
    int   add_student(StudentStore* s, int id, const char* name,
        const int* scores, int subject_count);
    Student* find_student_by_id(StudentStore* s, int id);
    Student* find_student_by_name(StudentStore* s, const char* name);
    int   update_student(StudentStore* s, int id,
        const int* new_scores, int subject_count);
    int   update_student_name(StudentStore* s, int id, const char* new_name);
    int   delete_student(StudentStore* s, int id);

    /* 정렬 */
    void  sort_by_average(StudentStore* s);
    void  sort_by_name(StudentStore* s);
    void  sort_by_average_then_name(StudentStore* s);
    void  sort_by_name_then_id(StudentStore* s);
    void  sort_by_id(StudentStore* s);

    /* 랭킹/통계 */
    void  recompute_ranks(StudentStore* s);
    void  compute_subject_stats(const StudentStore* s,
        SubjectStats out_stats[MAX_SUBJECTS]);

    /* 5주차 고급 검색 */
    size_t find_students_by_name_substr_ci(
        StudentStore* s, const char* needle,
        Student** out_array, size_t max_out);

    /* 6주차 리포트/머지 */
    void  print_student_pretty(const Student* st);
    int   export_report_txt(const char* path, const StudentStore* store);
    int   import_merge_csv(const char* path, StudentStore* store,
        int overwrite_existing,
        int* out_added, int* out_updated, int* out_skipped);

    /* 7주차: 가중치·컷 적용/JSON */
    void  recompute_all_averages(StudentStore* s, const GradeConfig* cfg); /* 평균 전원 재계산 */
    int   export_json(const char* path, const StudentStore* store,
        const GradeConfig* cfg);                              /* JSON 내보내기 */

#ifdef __cplusplus
}
#endif
#endif
