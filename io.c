#define _CRT_SECURE_NO_WARNINGS
#include "io.h"
#include <stdio.h>
#include <string.h>

void clear_input_buffer(void) { int c; while ((c = getchar()) != '\n' && c != EOF) {} }

int prompt_int(const char* msg, int min, int max) {
    int v; char line[128];
    while (1) {
        printf("%s", msg);
        if (!fgets(line, sizeof(line), stdin)) { printf("입력 오류.\n"); continue; }
        if (sscanf(line, "%d", &v) == 1 && v >= min && v <= max) return v;
        printf("유효한 정수(%d~%d)를 입력하세요.\n", min, max);
    }
}

float prompt_float(const char* msg, float min, float max) {
    float v; char line[128];
    while (1) {
        printf("%s", msg);
        if (!fgets(line, sizeof(line), stdin)) { printf("입력 오류.\n"); continue; }
        if (sscanf(line, "%f", &v) == 1 && v >= min && v <= max) return v;
        printf("유효한 실수(%.2f~%.2f)를 입력하세요.\n", min, max);
    }
}

void prompt_string(const char* msg, char* out, int out_size) {
    size_t len;
    while (1) {
        printf("%s", msg);
        if (!fgets(out, out_size, stdin)) { printf("입력 오류.\n"); continue; }
        len = strlen(out); if (len > 0 && out[len - 1] == '\n') out[len - 1] = '\0';
        if (out[0] == '\0') { printf("빈 문자열 불가.\n"); continue; }
        return;
    }
}

int prompt_yes_no(const char* msg) {
    char buf[8]; printf("%s (Y/N): ", msg);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    return (buf[0] == 'Y' || buf[0] == 'y') ? 1 : 0;
}

/* CSV 저장/불러오기 (6주차와 동일) */
int save_csv(const char* path, const StudentStore* store) {
    FILE* fp; size_t i;
#if defined(_MSC_VER)
    if (fopen_s(&fp, path, "w") != 0 || !fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#else
    fp = fopen(path, "w"); if (!fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#endif
    fprintf(fp, "id,name,subject_count,score1,score2,score3,score4,score5,average,rank\n");
    for (i = 0;i < store->size;++i) {
        const Student* st = &store->data[i];
        fprintf(fp, "%d,%s,%d,%d,%d,%d,%d,%d,%.2f,%d\n",
            st->id, st->name, st->subject_count,
            st->scores[0], st->scores[1], st->scores[2], st->scores[3], st->scores[4],
            st->average, st->rank);
    }
    fclose(fp); return 1;
}

int load_csv(const char* path, StudentStore* store) {
    FILE* fp; char line[512];
#if defined(_MSC_VER)
    if (fopen_s(&fp, path, "r") != 0 || !fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#else
    fp = fopen(path, "r"); if (!fp) { printf("파일 열기 실패: %s\n", path); return 0; }
#endif
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return 0; }
    store->size = 0;
    while (fgets(line, sizeof(line), fp)) {
        int id, sc1, sc2, sc3, sc4, sc5, cnt, rank; char name[MAX_NAME_LEN]; float avg;
#if defined(_MSC_VER)
        { int ok = sscanf_s(line, "%d,%49[^,],%d,%d,%d,%d,%d,%d,%f,%d",
            &id, name, (unsigned)MAX_NAME_LEN, &cnt, &sc1, &sc2, &sc3, &sc4, &sc5, &avg, &rank);
        if (ok == 10) {
#else
            { int ok = sscanf(line, "%d,%49[^,],%d,%d,%d,%d,%d,%d,%f,%d",
                &id, name, &cnt, &sc1, &sc2, &sc3, &sc4, &sc5, &avg, &rank);
            if (ok == 10) {
#endif
                int scores[MAX_SUBJECTS] = { sc1,sc2,sc3,sc4,sc5 };
                add_student(store, id, name, scores, cnt);
                /* avg/rank는 내부 규칙으로 재계산되므로 참고만 */
            }}
        }
    fclose(fp); return 1;
        }
