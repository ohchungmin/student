#include "model.h"                 /* Student, StudentStore 선언 */
#include <stdlib.h>                /* malloc/realloc/free/exit */
#include <string.h>                /* strcpy/strcmp 등 */
#include <stdio.h>                 /* printf/fprintf */

#define _CRT_SECURE_NO_WARNINGS  /* MSVC의 fopen/scanf 경고 끄기 */
#pragma warning(disable:4996)
static void ensure_capacity(StudentStore* s, size_t need) {
    if (s->capacity >= need) return;                 /* 이미 충분하면 아무것도 안 함 */

    {
        size_t newcap = (s->capacity == 0) ? 8 : s->capacity * 2;
        while (newcap < need) newcap *= 2;

        {
            Student* p = (Student*)realloc(s->data, newcap * sizeof(Student));
            if (!p) exit(EXIT_FAILURE);             /* 메모리 못 빌리면 그냥 종료 */
            s->data = p;
            s->capacity = newcap;
        }
    }
}

/* =========================================
   저장소를 '빈 상자'로 준비
   ========================================= */
void init_store(StudentStore* s) {
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

/* =========================================
   저장소 정리(메모리 반납)
   ========================================= */
void free_store(StudentStore* s) {
    free(s->data);
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

/* =========================================
   평균 계산기(합/과목수)
   ========================================= */
float compute_average(const int* scores, int subject_count) {
    int i, sum;
    if (subject_count <= 0) return 0.0f;
    sum = 0;
    for (i = 0; i < subject_count; ++i) sum += scores[i];
    return (float)sum / (float)subject_count;
}

/* =========================================
   평균으로 등급(A/B/C/D/F) 결정하기
   ========================================= */
const char* grade_of_average(float avg) {
    if (avg >= 90.0f) return "A";
    if (avg >= 80.0f) return "B";
    if (avg >= 70.0f) return "C";
    if (avg >= 60.0f) return "D";
    return "F";
}

/* =========================================
   [도우미] 이 학번이 이미 있는지 검사(있으면 1)
   ========================================= */
static int id_exists(StudentStore* s, int id) {
    size_t i;
    for (i = 0; i < s->size; ++i)
        if (s->data[i].id == id) return 1;
    return 0;
}

/* =========================================
   새 학생 추가(중복 학번이면 실패)
   ========================================= */
int add_student(StudentStore* s, int id, const char* name,
    const int* scores, int subject_count) {
    Student st;
    int i;

    if (!name || !scores) return 0;
    if (subject_count < 1 || subject_count > MAX_SUBJECTS) return 0;
    if (id_exists(s, id)) return 0;

    ensure_capacity(s, s->size + 1);

    st.id = id;
#if defined(_MSC_VER)
    strncpy_s(st.name, MAX_NAME_LEN, name, _TRUNCATE);
#else
    strncpy(st.name, name, MAX_NAME_LEN - 1);
    st.name[MAX_NAME_LEN - 1] = '\0';
#endif
    st.subject_count = subject_count;

    for (i = 0; i < subject_count; ++i) st.scores[i] = scores[i];
    for (; i < MAX_SUBJECTS; ++i)       st.scores[i] = 0;

    st.average = compute_average(st.scores, st.subject_count);
    st.rank = 0;

    s->data[s->size++] = st;
    recompute_ranks(s);
    return 1;
}

/* =========================================
   학번으로 학생 찾기(없으면 NULL)
   ========================================= */
Student* find_student_by_id(StudentStore* s, int id) {
    size_t i;
    for (i = 0; i < s->size; ++i)
        if (s->data[i].id == id) return &s->data[i];
    return NULL;
}

/* =========================================
   이름 '완전 일치'로 학생 찾기(첫 번째만)
   ========================================= */
Student* find_student_by_name(StudentStore* s, const char* name) {
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < s->size; ++i)
        if (strcmp(s->data[i].name, name) == 0)
            return &s->data[i];
    return NULL;
}

/* =========================================
   점수 수정(평균 다시, 등수 다시)
   ========================================= */
int update_student(StudentStore* s, int id,
    const int* new_scores, int subject_count) {
    Student* st;
    int i;

    if (!new_scores) return 0;
    if (subject_count < 1 || subject_count > MAX_SUBJECTS) return 0;

    st = find_student_by_id(s, id);
    if (!st) return 0;

    st->subject_count = subject_count;
    for (i = 0; i < subject_count; ++i) st->scores[i] = new_scores[i];
    for (; i < MAX_SUBJECTS; ++i)       st->scores[i] = 0;

    st->average = compute_average(st->scores, st->subject_count);
    recompute_ranks(s);
    return 1;
}

/* =========================================
   5주차: 이름만 수정
   ========================================= */
int update_student_name(StudentStore* s, int id, const char* new_name) {
    Student* st;
    if (!new_name) return 0;

    st = find_student_by_id(s, id);
    if (!st) return 0;

#if defined(_MSC_VER)
    strncpy_s(st->name, MAX_NAME_LEN, new_name, _TRUNCATE);
#else
    strncpy(st->name, new_name, MAX_NAME_LEN - 1);
    st->name[MAX_NAME_LEN - 1] = '\0';
#endif
    return 1;
}

/* =========================================
   학생 삭제(뒤의 것들을 한 칸씩 당겨 채우기)
   ========================================= */
int delete_student(StudentStore* s, int id) {
    size_t i, j;

    for (i = 0; i < s->size; ++i) {
        if (s->data[i].id == id) {
            for (j = i; j + 1 < s->size; ++j)
                s->data[j] = s->data[j + 1];
            s->size--;
            recompute_ranks(s);
            return 1;
        }
    }
    return 0;
}

/* =========================================
   정렬 비교기들
   ========================================= */
static int cmp_avg_desc(const void* a, const void* b) {
    const Student* sa = (const Student*)a;
    const Student* sb = (const Student*)b;
    if (sa->average < sb->average) return 1;
    if (sa->average > sb->average) return -1;
    return 0;
}
static int cmp_name_asc(const void* a, const void* b) {
    const Student* sa = (const Student*)a;
    const Student* sb = (const Student*)b;
    return strcmp(sa->name, sb->name);
}
static int cmp_id_asc(const void* a, const void* b) {
    const Student* sa = (const Student*)a;
    const Student* sb = (const Student*)b;
    if (sa->id < sb->id) return -1;
    if (sa->id > sb->id) return 1;
    return 0;
}
static int cmp_avg_desc_then_name_asc(const void* a, const void* b) {
    int first = cmp_avg_desc(a, b);
    if (first != 0) return first;
    return cmp_name_asc(a, b);
}
static int cmp_name_asc_then_id_asc(const void* a, const void* b) {
    int first = cmp_name_asc(a, b);
    if (first != 0) return first;
    return cmp_id_asc(a, b);
}

/* =========================================
   정렬 함수들
   ========================================= */
void sort_by_average(StudentStore* s) {
    if (s->size > 1)
        qsort(s->data, s->size, sizeof(Student), cmp_avg_desc);
}
void sort_by_name(StudentStore* s) {
    if (s->size > 1)
        qsort(s->data, s->size, sizeof(Student), cmp_name_asc);
}
void sort_by_average_then_name(StudentStore* s) {
    if (s->size > 1)
        qsort(s->data, s->size, sizeof(Student), cmp_avg_desc_then_name_asc);
}
void sort_by_name_then_id(StudentStore* s) {
    if (s->size > 1)
        qsort(s->data, s->size, sizeof(Student), cmp_name_asc_then_id_asc);
}
void sort_by_id(StudentStore* s) {
    if (s->size > 1)
        qsort(s->data, s->size, sizeof(Student), cmp_id_asc);
}

/* =========================================
   등수 다시 계산(나보다 평균 높은 사람 수 + 1)
   ========================================= */
void recompute_ranks(const StudentStore* s) {
    size_t i, j;
    for (i = 0; i < s->size; ++i) {
        int rank = 1;
        for (j = 0; j < s->size; ++j)
            if (s->data[j].average > s->data[i].average)
                rank++;
        s->data[i].rank = rank;
    }
}

/* =========================================
   과목별 통계(각 과목의 count/min/max/avg)
   ========================================= */
void compute_subject_stats(const StudentStore* s,
    SubjectStats out_stats[MAX_SUBJECTS]) {
    size_t i;
    int k;

    for (k = 0; k < MAX_SUBJECTS; ++k) {
        out_stats[k].count = 0;
        out_stats[k].min = 0;
        out_stats[k].max = 0;
        out_stats[k].average = 0.0f;
    }

    for (k = 0; k < MAX_SUBJECTS; ++k) {
        int sum = 0, minv = 0, maxv = 0, first = 1;

        for (i = 0; i < s->size; ++i) {
            if (s->data[i].subject_count > k) {
                int sc = s->data[i].scores[k];

                if (first) {
                    minv = maxv = sc;
                    first = 0;
                }
                else {
                    if (sc < minv) minv = sc;
                    if (sc > maxv) maxv = sc;
                }
                sum += sc;
                out_stats[k].count++;
            }
        }

        if (out_stats[k].count > 0) {
            out_stats[k].min = minv;
            out_stats[k].max = maxv;
            out_stats[k].average = (float)sum / out_stats[k].count;
        }
    }
}

/* =========================================
   부분 문자열 검색(대소문자 무시) 도우미들
   ========================================= */
static char tolower_c(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

static int contains_substr_ci(const char* hay, const char* nee) {
    const char* p;
    size_t i;

    if (!hay || !nee) return 0;
    if (*nee == '\0') return 1;

    for (p = hay; *p != '\0'; ++p) {
        for (i = 0;; ++i) {
            char hc = tolower_c(p[i]);
            char nc = tolower_c(nee[i]);
            if (nee[i] == '\0') return 1;
            if (p[i] == '\0') return 0;
            if (hc != nc) break;
        }
    }
    return 0;
}

/* needle 이 들어있는 학생들을 찾아 포인터 배열로 반환 */
size_t find_students_by_name_substr_ci(StudentStore* s,
    const char* needle,
    Student** out_array, size_t max_out) {
    size_t i, out = 0;
    if (!needle || !out_array || max_out == 0) return 0;

    for (i = 0; i < s->size; ++i) {
        if (contains_substr_ci(s->data[i].name, needle)) {
            if (out < max_out) {
                out_array[out] = &s->data[i];
                ++out;
            }
            else break;
        }
    }
    return out;
}

/* =========================================
   학생 1명 성적표 예쁘게 출력
   ========================================= */
void print_student_pretty(const Student* st) {
    size_t i;
    if (!st) { printf("(학생 없음)\n"); return; }

    printf("\n==== 학생 성적표 ====\n");
    printf("학번: %d\n", st->id);
    printf("이름: %s\n", st->name);
    printf("과목 수: %d\n", st->subject_count);

    printf("점수: ");
    for (i = 0; i < (size_t)st->subject_count; ++i)
        printf("%d ", st->scores[i]);

    printf("\n평균: %.2f\n", st->average);
    printf("등급: %s\n", grade_of_average(st->average));
    printf("등수: %d\n", st->rank);
    printf("=====================\n");
}

/* =========================================
   전체 리포트를 TXT 파일로 내보내기
   ========================================= */
int export_report_txt(const char* path, const StudentStore* store) {
    FILE* fp;
    size_t i, j;

    if (!path || !store) return 0;

    fp = fopen(path, "w");
    if (!fp) {
        printf("파일 열기 실패: %s\n", path);
        return 0;
    }

    fprintf(fp, "==== 학생 전체 리포트 ====\n");
    fprintf(fp, "총 인원: %u명\n\n", (unsigned)store->size);

    fprintf(fp, "ID\t이름\t과목수\t점수들\t\t평균\t등급\t등수\n");
    for (i = 0; i < store->size; ++i) {
        const Student* st = &store->data[i];

        fprintf(fp, "%d\t%s\t%d\t", st->id, st->name, st->subject_count);
        for (j = 0; j < (size_t)st->subject_count; ++j)
            fprintf(fp, "%d ", st->scores[j]);
        if (st->subject_count < MAX_SUBJECTS) fprintf(fp, "\t");

        fprintf(fp, "\t%.2f\t%s\t%d\n",
            st->average, grade_of_average(st->average), st->rank);
    }

    fclose(fp);
    return 1;
}

/* =========================================
   CSV "머지" 불러오기
   ========================================= */
int import_merge_csv(const char* path, StudentStore* store,
    int overwrite_existing,
    int* out_added, int* out_updated, int* out_skipped) {
    FILE* fp;
    char line[512];

    int added = 0;
    int updated = 0;
    int skipped = 0;

    if (!path || !store) return 0;

    fp = fopen(path, "r");
    if (!fp) {
        printf("파일 열기 실패: %s\n", path);
        return 0;
    }

    /* 헤더 한 줄 버리기 */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        int id, sc1, sc2, sc3, sc4, sc5;
        int subject_count, rank_dummy;
        char  name[MAX_NAME_LEN];
        float avg_dummy;
        int ok = sscanf(line,
            "%d,%49[^,],%d,%d,%d,%d,%d,%d,%f,%d",
            &id, name, &subject_count,
            &sc1, &sc2, &sc3, &sc4, &sc5,
            &avg_dummy, &rank_dummy);

        if (ok == 10) {
            int scores[MAX_SUBJECTS];
            Student* exist;

            scores[0] = sc1;
            scores[1] = sc2;
            scores[2] = sc3;
            scores[3] = sc4;
            scores[4] = sc5;

            exist = find_student_by_id(store, id);

            if (!exist) {
                if (add_student(store, id, name, scores, subject_count))
                    added++;
            }
            else {
                if (overwrite_existing) {
                    update_student_name(store, id, name);
                    update_student(store, id, scores, subject_count);
                    updated++;
                }
                else {
                    skipped++;
                }
            }
        }
        /* 포맷이 잘못된 줄은 무시 */
    }

    fclose(fp);

    if (out_added)   *out_added = added;
    if (out_updated) *out_updated = updated;
    if (out_skipped) *out_skipped = skipped;

    return 1;
}
