#define _CRT_SECURE_NO_WARNINGS
#include "model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== 내부 유틸 ========== */
static void ensure_capacity(StudentStore* s, size_t need) {
    if (s->capacity >= need) return;
    { size_t cap = s->capacity ? s->capacity * 2 : 8;
    while (cap < need) cap *= 2;
    { Student* p = (Student*)realloc(s->data, cap * sizeof(Student));
    if (!p) exit(EXIT_FAILURE);
    s->data = p; s->capacity = cap; } }
}

/* ========== 수명주기 & 설정 ========== */
void init_store(StudentStore* s) { s->data = NULL; s->size = 0; s->capacity = 0; }
void free_store(StudentStore* s) { free(s->data); s->data = NULL; s->size = 0; s->capacity = 0; }

void init_config(GradeConfig* cfg) {
    int i; for (i = 0;i < MAX_SUBJECTS;++i) cfg->subject_weights[i] = 1.0f;
    cfg->pass_threshold = 60.0f;
}

/* ========== 평균/등급 ========== */
float compute_average(const int* scores, int n) {
    int i, sum = 0; if (n <= 0) return 0.0f; for (i = 0;i < n;++i) sum += scores[i];
    return (float)sum / (float)n;
}

/* 7주차: 과목별 가중치 평균 */
float compute_weighted_average(const int* scores, int n, const float* w) {
    int i; float ws = 0.0f, sum = 0.0f; if (n <= 0) return 0.0f;
    for (i = 0;i < n;++i) { sum += scores[i] * w[i]; ws += w[i]; }
    if (ws <= 0.0f) return compute_average(scores, n);
    return sum / ws;
}

const char* grade_of_average(float a) {
    if (a >= 90.f) return "A";
    if (a >= 80.f) return "B";
    if (a >= 70.f) return "C";
    if (a >= 60.f) return "D";
    return "F";
}

/* 7주차: 합격/불합격 라벨 */
const char* pass_label(float avg, float cut) {
    return (avg >= cut) ? "PASS" : "FAIL";
}

/* ========== CRUD ========== */
static int id_exists(StudentStore* s, int id) {
    size_t i; for (i = 0;i < s->size;++i) if (s->data[i].id == id) return 1; return 0;
}

int add_student(StudentStore* s, int id, const char* name, const int* scores, int n) {
    Student st; int i;
    if (!name || !scores) return 0;
    if (n<1 || n>MAX_SUBJECTS) return 0;
    if (id_exists(s, id)) return 0;

    ensure_capacity(s, s->size + 1);
    st.id = id;
#if defined(_MSC_VER)
    strncpy_s(st.name, MAX_NAME_LEN, name, _TRUNCATE);
#else
    strncpy(st.name, name, MAX_NAME_LEN - 1); st.name[MAX_NAME_LEN - 1] = '\0';
#endif
    st.subject_count = n;
    for (i = 0;i < n;++i) st.scores[i] = scores[i];
    for (;i < MAX_SUBJECTS;++i) st.scores[i] = 0;
    st.average = compute_average(st.scores, st.subject_count); /* 기본 평균, 이후 재계산 가능 */
    st.rank = 0;
    s->data[s->size++] = st;
    return 1;
}

Student* find_student_by_id(StudentStore* s, int id) {
    size_t i; for (i = 0;i < s->size;++i) if (s->data[i].id == id) return &s->data[i];
    return NULL;
}
Student* find_student_by_name(StudentStore* s, const char* name) {
    size_t i; if (!name) return NULL;
    for (i = 0;i < s->size;++i) if (strcmp(s->data[i].name, name) == 0) return &s->data[i];
    return NULL;
}
int update_student(StudentStore* s, int id, const int* sc, int n) {
    Student* st = find_student_by_id(s, id); int i; if (!st || !sc) return 0;
    if (n<1 || n>MAX_SUBJECTS) return 0;
    st->subject_count = n;
    for (i = 0;i < n;++i) st->scores[i] = sc[i];
    for (;i < MAX_SUBJECTS;++i) st->scores[i] = 0;
    st->average = compute_average(st->scores, st->subject_count);
    return 1;
}
int update_student_name(StudentStore* s, int id, const char* nm) {
    Student* st = find_student_by_id(s, id); if (!st || !nm) return 0;
#if defined(_MSC_VER)
    strncpy_s(st->name, MAX_NAME_LEN, nm, _TRUNCATE);
#else
    strncpy(st->name, nm, MAX_NAME_LEN - 1); st->name[MAX_NAME_LEN - 1] = '\0';
#endif
    return 1;
}
int delete_student(StudentStore* s, int id) {
    size_t i, j; for (i = 0;i < s->size;++i) if (s->data[i].id == id) {
        for (j = i;j + 1 < s->size;++j) s->data[j] = s->data[j + 1]; s->size--; return 1;
    }
    return 0;
}

/* ========== 정렬/랭킹/통계 ========== */
static int cmp_avg_desc(const void* a, const void* b) {
    const Student* A = (const Student*)a; const Student* B = (const Student*)b;
    if (A->average < B->average) return 1; if (A->average > B->average) return -1; return 0;
}
static int cmp_name_asc(const void* a, const void* b) {
    const Student* A = (const Student*)a; const Student* B = (const Student*)b;
    return strcmp(A->name, B->name);
}
static int cmp_id_asc(const void* a, const void* b) {
    const Student* A = (const Student*)a; const Student* B = (const Student*)b;
    if (A->id < B->id) return -1; if (A->id > B->id) return 1; return 0;
}
static int cmp_avg_desc_name_asc(const void* a, const void* b) {
    int c = cmp_avg_desc(a, b); return c ? c : cmp_name_asc(a, b);
}
static int cmp_name_asc_id_asc(const void* a, const void* b) {
    int c = cmp_name_asc(a, b); return c ? c : cmp_id_asc(a, b);
}
void sort_by_average(StudentStore* s) { if (s->size > 1) qsort(s->data, s->size, sizeof(Student), cmp_avg_desc); }
void sort_by_name(StudentStore* s) { if (s->size > 1) qsort(s->data, s->size, sizeof(Student), cmp_name_asc); }
void sort_by_average_then_name(StudentStore* s) { if (s->size > 1) qsort(s->data, s->size, sizeof(Student), cmp_avg_desc_name_asc); }
void sort_by_name_then_id(StudentStore* s) { if (s->size > 1) qsort(s->data, s->size, sizeof(Student), cmp_name_asc_id_asc); }
void sort_by_id(StudentStore* s) { if (s->size > 1) qsort(s->data, s->size, sizeof(Student), cmp_id_asc); }

void recompute_ranks(StudentStore* s) {
    size_t i, j;
    for (i = 0;i < s->size;++i) {
        int r = 1; for (j = 0;j < s->size;++j) if (s->data[j].average > s->data[i].average) r++;
        s->data[i].rank = r;
    }
}

void compute_subject_stats(const StudentStore* s, SubjectStats out[MAX_SUBJECTS]) {
    int k; size_t i;
    for (k = 0;k < MAX_SUBJECTS;++k) { out[k].count = 0; out[k].min = 0; out[k].max = 0; out[k].average = 0.0f; }
    for (k = 0;k < MAX_SUBJECTS;++k) {
        int first = 1, sum = 0, minv = 0, maxv = 0;
        for (i = 0;i < s->size;++i) {
            if (s->data[i].subject_count > k) {
                int sc = s->data[i].scores[k];
                if (first) { minv = maxv = sc; first = 0; }
                else { if (sc < minv)minv = sc; if (sc > maxv)maxv = sc; }
                sum += sc; out[k].count++;
            }
        }
        if (out[k].count > 0) { out[k].min = minv; out[k].max = maxv; out[k].average = (float)sum / out[k].count; }
    }
}

/* 5주차 검색(대소문자 무시) */
static char tolower_c(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static int contains_substr_ci(const char* h, const char* n) {
    const char* p; size_t i; if (!h || !n) return 0; if (*n == '\0') return 1;
    for (p = h; *p; ++p) {
        for (i = 0;;++i) {
            char hc = tolower_c(p[i]), nc = tolower_c(n[i]);
            if (n[i] == '\0') return 1; if (p[i] == '\0') return 0; if (hc != nc) break;
        }
    }
    return 0;
}
size_t find_students_by_name_substr_ci(StudentStore* s, const char* needle, Student** out, size_t max_out) {
    size_t i, c = 0; if (!needle || !out || !max_out) return 0;
    for (i = 0;i < s->size;++i) if (contains_substr_ci(s->data[i].name, needle)) { if (c < max_out) out[c++] = &s->data[i]; else break; }
    return c;
}

/* 6주차: 개인 성적표/리포트/머지 */
void print_student_pretty(const Student* st) {
    size_t i; if (!st) { printf("(학생 없음)\n"); return; }
    printf("\n==== 학생 성적표 ====\n");
    printf("학번: %d\n이름: %s\n과목 수: %d\n점수: ", st->id, st->name, st->subject_count);
    for (i = 0;i < (size_t)st->subject_count;++i) printf("%d ", st->scores[i]);
    printf("\n평균: %.2f\n등급: %s\n등수: %d\n", st->average, grade_of_average(st->average), st->rank);
    printf("=====================\n");
}

int export_report_txt(const char* path, const StudentStore* store) {
    FILE* fp; size_t i, j; if (!path || !store) return 0;
#if defined(_MSC_VER)
    if (fopen_s(&fp, path, "w") != 0 || !fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#else
    fp = fopen(path, "w"); if (!fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#endif
    fprintf(fp, "==== 학생 전체 리포트 ====\n총 인원: %u명\n\n", (unsigned)store->size);
    fprintf(fp, "ID\t이름\t과목수\t점수들\t\t평균\t등급\t등수\n");
    for (i = 0;i < store->size;++i) {
        const Student* st = &store->data[i];
        fprintf(fp, "%d\t%s\t%d\t", st->id, st->name, st->subject_count);
        for (j = 0;j < (size_t)st->subject_count;++j) fprintf(fp, "%d ", st->scores[j]);
        fprintf(fp, "\t%.2f\t%s\t%d\n", st->average, grade_of_average(st->average), st->rank);
    }
    fclose(fp); return 1;
}

int import_merge_csv(const char* path, StudentStore* store,
    int overwrite_existing, int* out_a, int* out_u, int* out_s) {
    FILE* fp; char line[512]; int add = 0, upd = 0, sk = 0;
#if defined(_MSC_VER)
    if (fopen_s(&fp, path, "r") != 0 || !fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#else
    fp = fopen(path, "r"); if (!fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#endif
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return 0; }
    while (fgets(line, sizeof(line), fp)) {
        int id, sc1, sc2, sc3, sc4, sc5, cnt, rank_dummy; char name[MAX_NAME_LEN]; float avg_dummy;
#if defined(_MSC_VER)
        { int ok = sscanf_s(line, "%d,%49[^,],%d,%d,%d,%d,%d,%d,%f,%d",
            &id, name, (unsigned)MAX_NAME_LEN, &cnt, &sc1, &sc2, &sc3, &sc4, &sc5, &avg_dummy, &rank_dummy);
        if (ok == 10) {
#else
            { int ok = sscanf(line, "%d,%49[^,],%d,%d,%d,%d,%d,%d,%f,%d",
                &id, name, &cnt, &sc1, &sc2, &sc3, &sc4, &sc5, &avg_dummy, &rank_dummy);
            if (ok == 10) {
#endif
                int scores[MAX_SUBJECTS] = { sc1,sc2,sc3,sc4,sc5 }; Student* ex = find_student_by_id(store, id);
                if (!ex) { if (add_student(store, id, name, scores, cnt)) add++; }
                else if (overwrite_existing) { update_student_name(store, id, name); update_student(store, id, scores, cnt); upd++; }
                else { sk++; }
            }}
        }
    fclose(fp);
    if (out_a)*out_a = add; if (out_u)*out_u = upd; if (out_s)*out_s = sk; return 1;
        }

/* ========== 7주차: 평균 일괄 재계산 & JSON ========== */
void recompute_all_averages(StudentStore * s, const GradeConfig * cfg) {
    size_t i;
    for (i = 0;i < s->size;++i) {
        s->data[i].average = compute_weighted_average(
            s->data[i].scores, s->data[i].subject_count, cfg->subject_weights);
    }
    recompute_ranks(s);
}

int export_json(const char* path, const StudentStore * store, const GradeConfig * cfg) {
    FILE* fp; size_t i, j; if (!path || !store || !cfg) return 0;
#if defined(_MSC_VER)
    if (fopen_s(&fp, path, "w") != 0 || !fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#else
    fp = fopen(path, "w"); if (!fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#endif
    /* 간단 JSON: 객체 배열 + 구성(가중치/컷) 포함 */
    fprintf(fp, "{\n  \"config\": { \"weights\": [");
    for (j = 0;j < MAX_SUBJECTS;++j) { fprintf(fp, "%s%.2f", (j ? ", " : ""), cfg->subject_weights[j]); }
    fprintf(fp, "], \"pass_cut\": %.2f },\n", cfg->pass_threshold);

    fprintf(fp, "  \"students\": [\n");
    for (i = 0;i < store->size;++i) {
        const Student* st = &store->data[i];
        fprintf(fp, "    { \"id\": %d, \"name\": \"%s\", \"subjects\": %d, \"scores\": [",
            st->id, st->name, st->subject_count);
        for (j = 0;j < (size_t)st->subject_count;++j) fprintf(fp, "%s%d", (j ? ", " : ""), st->scores[j]);
        fprintf(fp, "], \"average\": %.2f, \"grade\": \"%s\", \"rank\": %d, \"pass\": \"%s\" }%s\n",
            st->average, grade_of_average(st->average), st->rank,
            pass_label(st->average, cfg->pass_threshold),
            (i + 1 < store->size ? "," : ""));
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp); return 1;
}
