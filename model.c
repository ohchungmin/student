// model.c : 학생 로직 구현
#define _CRT_SECURE_NO_WARNINGS
#include "model.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

// ---------- 내부 비교 헬퍼 ----------
static int cmp_name(const void* a, const void* b) {
    const Student* x = (const Student*)a;
    const Student* y = (const Student*)b;
    return strcmp(x->name, y->name);
}
static int cmp_total(const void* a, const void* b) {
    const Student* x = (const Student*)a;
    const Student* y = (const Student*)b;
    return (x->total - y->total);
}
static int cmp_avg(const void* a, const void* b) {
    const Student* x = (const Student*)a;
    const Student* y = (const Student*)b;
    if (x->avg < y->avg) return -1;
    if (x->avg > y->avg) return  1;
    return 0;
}
static void tolower_inplace(char* s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

// ---------- 기본 ----------
void store_init(StudentStore* s) {
    s->count = 0;
    s->weights.w_kor = 1.0 / 3; s->weights.w_eng = 1.0 / 3; s->weights.w_math = 1.0 / 3;
    s->pass_rule.pass_cut = 60.0;
}
char calc_grade_from_avg(double avg) {
    if (avg >= 90) return 'A';
    if (avg >= 80) return 'B';
    if (avg >= 70) return 'C';
    if (avg >= 60) return 'D';
    return 'F';
}
void store_recalc_one(Student* p, const GradeWeights* w) {
    // 가중 평균 = w_kor*kor + w_eng*eng + w_math*math
    double weighted = p->kor * w->w_kor + p->eng * w->w_eng + p->math * w->w_math;
    p->total = p->kor + p->eng + p->math;
    p->avg = weighted * 1.0;     // 이미 가중치 합이 1.0이 되도록 설정
    p->grade = calc_grade_from_avg(p->avg);
}
void store_recalc_all(StudentStore* s) {
    for (int i = 0;i < s->count;i++) store_recalc_one(&s->arr[i], &s->weights);
}

// ---------- CRUD ----------
int store_find_index_by_id(const StudentStore* s, int id) {
    for (int i = 0;i < s->count;i++) if (s->arr[i].id == id) return i;
    return -1;
}
int store_add(StudentStore* s, Student in) {
    if (s->count >= MAX_STU) return 0;
    if (store_find_index_by_id(s, in.id) >= 0) return 0;
    in.deleted = 0;
    store_recalc_one(&in, &s->weights);
    s->arr[s->count++] = in;
    return 1;
}
int store_edit_scores(StudentStore* s, int id, int kor, int eng, int math) {
    int idx = store_find_index_by_id(s, id);
    if (idx < 0) return 0;
    Student* p = &s->arr[idx];
    if (p->deleted) return 0;
    p->kor = kor; p->eng = eng; p->math = math;
    store_recalc_one(p, &s->weights);
    return 1;
}
int store_edit_name(StudentStore* s, int id, const char* name) {
    int idx = store_find_index_by_id(s, id);
    if (idx < 0) return 0;
    Student* p = &s->arr[idx];
    if (p->deleted) return 0;
    strncpy(p->name, name, NAME_LEN - 1);
    p->name[NAME_LEN - 1] = '\0';
    return 1;
}
int store_soft_delete(StudentStore* s, int id) {
    int idx = store_find_index_by_id(s, id);
    if (idx < 0) return 0;
    s->arr[idx].deleted = 1;
    return 1;
}
int store_restore(StudentStore* s, int id) {
    int idx = store_find_index_by_id(s, id);
    if (idx < 0) return 0;
    s->arr[idx].deleted = 0;
    return 1;
}

// ---------- 조회/검색 ----------
void print_student_header(void) {
    printf("학번   이름            국어  영어  수학  총점  평균   학점  상태\n");
    printf("---------------------------------------------------------------\n");
}
void print_student_row(const Student* p) {
    printf("%-6d %-14s %4d  %4d  %4d  %4d  %6.2f   %c   %s\n",
        p->id, p->name, p->kor, p->eng, p->math, p->total, p->avg, p->grade,
        p->deleted ? "삭제" : "정상");
}
void store_list_active(const StudentStore* s) {
    print_student_header();
    int shown = 0;
    for (int i = 0;i < s->count;i++) {
        if (s->arr[i].deleted) continue;
        print_student_row(&s->arr[i]); shown++;
    }
    if (!shown) puts("(표시할 학생 없음)");
}
int store_count_active(const StudentStore* s) {
    int c = 0; for (int i = 0;i < s->count;i++) if (!s->arr[i].deleted) c++; return c;
}
int store_search_id(const StudentStore* s, int id, Student* out) {
    int idx = store_find_index_by_id(s, id);
    if (idx < 0 || s->arr[idx].deleted) return 0;
    if (out) *out = s->arr[idx];
    return 1;
}
int store_search_name_exact(const StudentStore* s, const char* name, Student* out_list, int cap) {
    int n = 0;
    for (int i = 0;i < s->count;i++) {
        if (s->arr[i].deleted) continue;
        if (strcmp(s->arr[i].name, name) == 0) {
            if (out_list && n < cap) out_list[n] = s->arr[i];
            n++;
        }
    }
    return n;
}
int store_search_name_partial_ci(const StudentStore* s, const char* key, Student* out_list, int cap) {
    char kbuf[NAME_LEN]; strncpy(kbuf, key, NAME_LEN - 1); kbuf[NAME_LEN - 1] = '\0'; tolower_inplace(kbuf);
    int n = 0;
    for (int i = 0;i < s->count;i++) {
        if (s->arr[i].deleted) continue;
        char nbuf[NAME_LEN]; strncpy(nbuf, s->arr[i].name, NAME_LEN - 1); nbuf[NAME_LEN - 1] = '\0'; tolower_inplace(nbuf);
        if (strstr(nbuf, kbuf)) {
            if (out_list && n < cap) out_list[n] = s->arr[i];
            n++;
        }
    }
    return n;
}

// ---------- 통계 ----------
void store_subject_stats(const StudentStore* s, const char* label) {
    int n = 0, maxK = -1, maxE = -1, maxM = -1, minK = 101, minE = 101, minM = 101;
    long sumK = 0, sumE = 0, sumM = 0;
    for (int i = 0;i < s->count;i++) {
        const Student* p = &s->arr[i]; if (p->deleted) continue;
        n++;
        if (p->kor > maxK) maxK = p->kor; if (p->kor < minK) minK = p->kor; sumK += p->kor;
        if (p->eng > maxE) maxE = p->eng; if (p->eng < minE) minE = p->eng; sumE += p->eng;
        if (p->math > maxM) maxM = p->math; if (p->math < minM) minM = p->math; sumM += p->math;
    }
    printf("[%s] 표본=%d\n", label, n);
    if (n == 0) { puts("데이터 없음."); return; }
    printf("국어  평균 %.2f, 최댓값 %d, 최솟값 %d\n", sumK * 1.0 / n, maxK, minK);
    printf("영어  평균 %.2f, 최댓값 %d, 최솟값 %d\n", sumE * 1.0 / n, maxE, minE);
    printf("수학  평균 %.2f, 최댓값 %d, 최솟값 %d\n", sumM * 1.0 / n, maxM, minM);
}

// ---------- 정렬 ----------
static void reverse(Student* a, int n) { for (int i = 0;i < n / 2;i++) { Student t = a[i]; a[i] = a[n - 1 - i]; a[n - 1 - i] = t; } }
void store_sort(StudentStore* s, SortKey key, int ascending) {
    // 삭제 포함 정렬 후에 그대로 유지. 출력 시에는 deleted 제외.
    if (key == SORT_BY_NAME)       qsort(s->arr, s->count, sizeof(Student), cmp_name);
    else if (key == SORT_BY_TOTAL) qsort(s->arr, s->count, sizeof(Student), cmp_total);
    else                         qsort(s->arr, s->count, sizeof(Student), cmp_avg);
    if (!ascending) reverse(s->arr, s->count);
}
static int cmp_multi(const void* A, const void* B, void* ctx) {
    // ctx = int flags[3] : key1, asc1, key2, asc2
    int* f = (int*)ctx;
    const Student* x = (const Student*)A; const Student* y = (const Student*)B;
    int k1 = f[0], a1 = f[1], k2 = f[2], a2 = f[3];
    int r = 0;
    if (k1 == SORT_BY_NAME) r = strcmp(x->name, y->name);
    else if (k1 == SORT_BY_TOTAL) r = (x->total - y->total);
    else { if (x->avg < y->avg) r = -1; else if (x->avg > y->avg) r = 1; else r = 0; }
    if (!a1) r = -r;
    if (r != 0) return r;
    if (k2 == SORT_BY_NAME) r = strcmp(x->name, y->name);
    else if (k2 == SORT_BY_TOTAL) r = (x->total - y->total);
    else { if (x->avg < y->avg) r = -1; else if (x->avg > y->avg) r = 1; else r = 0; }
    if (!a2) r = -r;
    return r;
}
void store_sort_multi(StudentStore* s, SortKey key1, int asc1, SortKey key2, int asc2) {
#if defined(_MSC_VER) && _MSC_VER < 1900
    // MSVC 구버전에는 qsort_s 시그니처 차이. 단순 2단계 정렬로 대체.
    store_sort(s, key2, asc2);
    store_sort(s, key1, asc1);
#else
    int flags[4] = { (int)key1, asc1, (int)key2, asc2 };
    qsort_s(s->arr, s->count, sizeof(Student), cmp_multi, flags);
#endif
}

// ---------- Top-N ----------
int store_top_n(const StudentStore* s, int n, Student* out_list, int cap) {
    // 평균 기준 내림차순 Top-N
    Student tmp[MAX_STU];
    int m = 0;
    for (int i = 0;i < s->count;i++) if (!s->arr[i].deleted) tmp[m++] = s->arr[i];
    qsort(tmp, m, sizeof(Student), cmp_avg);
    // cmp_avg는 오름차순. 뒤에서 n개를 꺼내거나 reverse.
    for (int i = 0;i < m / 2;i++) { Student t = tmp[i]; tmp[i] = tmp[m - 1 - i]; tmp[m - 1 - i] = t; }
    if (n > m) n = m;
    for (int i = 0;i < n && i < cap;i++) out_list[i] = tmp[i];
    return n;
}

// ---------- 설정 ----------
void store_set_weights(StudentStore* s, double w_kor, double w_eng, double w_math) {
    double sum = w_kor + w_eng + w_math;
    if (sum <= 0) sum = 1.0;
    s->weights.w_kor = w_kor / sum;
    s->weights.w_eng = w_eng / sum;
    s->weights.w_math = w_math / sum;
    store_recalc_all(s);
}
void store_set_pass_cut(StudentStore* s, double cut) {
    s->pass_rule.pass_cut = cut;
}

// ---------- 합격 ----------
int is_pass(const Student* st, const PassRule* rule) {
    return st->avg >= rule->pass_cut;
}

// ---------- 단일 리포트 ----------
void print_single_report_card(const Student* p, const PassRule* rule) {
    puts("----------- 성적표 -----------");
    printf("학번: %d\n이름: %s\n", p->id, p->name);
    printf("국어: %d, 영어: %d, 수학: %d\n", p->kor, p->eng, p->math);
    printf("총점: %d, 평균: %.2f, 학점: %c\n", p->total, p->avg, p->grade);
    printf("합격 여부: %s (컷=%.1f)\n", is_pass(p, rule) ? "합격" : "불합격", rule->pass_cut);
    puts("------------------------------");
}
