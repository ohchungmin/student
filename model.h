#pragma once
// model.h : 학생/설정/연산 정의와 선언

#ifndef MODEL_H
#define MODEL_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    // ---------- 상수 ----------
#define NAME_LEN        32          // 이름 최대 길이
#define MAX_STU         1000        // 최대 학생 수

// ---------- 자료형 ----------
    typedef struct {
        int   id;                       // 학번(고유키)
        char  name[NAME_LEN];           // 이름
        int   kor, eng, math;           // 점수
        int   total;                    // 총점(파생)
        double avg;                     // 평균(파생)
        char  grade;                    // 학점(파생)
        int   deleted;                  // 1이면 소프트 삭제
    } Student;

    typedef struct {
        // 과목별 가중치(합이 1.0이 되도록 사용자가 조정)
        double w_kor;
        double w_eng;
        double w_math;
    } GradeWeights;

    typedef struct {
        // 합격 컷(평균 기준). 예: 60.0
        double pass_cut;
    } PassRule;

    typedef struct {
        Student arr[MAX_STU];           // 저장소
        int     count;                  // 사용 중인 슬롯 수(삭제 포함)
        GradeWeights weights;           // 가중치
        PassRule     pass_rule;         // 합격 컷
    } StudentStore;

    // ---------- 기본 유틸 ----------
    void store_init(StudentStore* s);
    void store_recalc_one(Student* p, const GradeWeights* w);
    void store_recalc_all(StudentStore* s);

    // ---------- CRUD ----------
    int  store_find_index_by_id(const StudentStore* s, int id);
    int  store_add(StudentStore* s, Student in);                   // 성공 1, 실패 0
    int  store_edit_scores(StudentStore* s, int id, int kor, int eng, int math);
    int  store_edit_name(StudentStore* s, int id, const char* name);
    int  store_soft_delete(StudentStore* s, int id);               // 삭제 1/실패 0
    int  store_restore(StudentStore* s, int id);                   // 복구 1/실패 0

    // ---------- 조회/검색 ----------
    void store_list_active(const StudentStore* s);                 // 콘솔 출력
    int  store_count_active(const StudentStore* s);
    int  store_search_id(const StudentStore* s, int id, Student* out);            // 찾으면 1
    int  store_search_name_exact(const StudentStore* s, const char* name, Student* out_list, int cap);
    int  store_search_name_partial_ci(const StudentStore* s, const char* key, Student* out_list, int cap);

    // ---------- 통계 ----------
    void store_subject_stats(const StudentStore* s, const char* label);

    // ---------- 정렬 ----------
    typedef enum { SORT_BY_AVG = 0, SORT_BY_NAME = 1, SORT_BY_TOTAL = 2 } SortKey;
    void store_sort(StudentStore* s, SortKey key, int ascending);
    void store_sort_multi(StudentStore* s, SortKey key1, int asc1, SortKey key2, int asc2);

    // ---------- Top-N ----------
    int  store_top_n(const StudentStore* s, int n, Student* out_list, int cap);

    // ---------- 설정 ----------
    void store_set_weights(StudentStore* s, double w_kor, double w_eng, double w_math);
    void store_set_pass_cut(StudentStore* s, double cut);

    // ---------- 학점/합격 ----------
    char calc_grade_from_avg(double avg);
    int   is_pass(const Student* st, const PassRule* rule);

    // ---------- 단일 출력 ----------
    void print_student_header(void);
    void print_student_row(const Student* p);
    void print_single_report_card(const Student* p, const PassRule* rule);

#ifdef __cplusplus
}
#endif

#endif // MODEL_H
