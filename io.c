#define _CRT_SECURE_NO_WARNINGS          /* MSVC의 fopen 경고 무시용 */

#include "io.h"
#include <stdio.h>
#include <string.h>

/* 입력 버퍼 비우기 — fgets를 쓰다가 꼬였을 때 사용 */
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* 남은 글자 버리기 */
    }
}

/* 정수 안전 입력 — 범위 벗어나면 재요청 */
int prompt_int(const char* msg, int min, int max) {
    int value;
    char line[128];

    while (1) {
        printf("%s", msg);  /* 안내 문구 */
        if (!fgets(line, sizeof(line), stdin)) {
            printf("입력 오류. 다시 시도하세요.\n");
            continue;
        }
        if (sscanf(line, "%d", &value) == 1 &&
            value >= min && value <= max) {
            return value;
        }
        printf("유효한 정수를 입력하세요 (%d ~ %d).\n", min, max);
    }
}

/* 문자열 안전 입력 — 끝의 엔터 제거, 빈 문자열이면 재요청 */
void prompt_string(const char* msg, char* out, int out_size) {
    size_t len;

    while (1) {
        printf("%s", msg);
        if (!fgets(out, out_size, stdin)) {
            printf("입력 오류. 다시 시도하세요.\n");
            continue;
        }

        len = strlen(out);
        if (len > 0 && out[len - 1] == '\n')
            out[len - 1] = '\0';  /* 엔터 제거 */

        if (strlen(out) == 0) {
            printf("빈 문자열은 허용되지 않습니다.\n");
            continue;
        }
        return;
    }
}

/* 예/아니오 간단 질문 — 'Y'/'y'면 1, 나머지는 0 */
int prompt_yes_no(const char* msg) {
    char buf[8];
    printf("%s (Y/N): ", msg);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    if (buf[0] == 'Y' || buf[0] == 'y') return 1;
    return 0;
}

/* CSV 저장: 헤더 1줄 + 학생 N줄 */
int save_csv(const char* path, const StudentStore* store) {
    FILE* fp;
    size_t i;

    fp = fopen(path, "w");
    if (!fp) {
        printf("파일 열기 실패: %s\n", path);
        return 0;
    }

    fprintf(fp, "id,name,subject_count,score1,score2,score3,score4,score5,average,rank\n");

    for (i = 0; i < store->size; ++i) {
        const Student* st = &store->data[i];
        fprintf(fp, "%d,%s,%d,%d,%d,%d,%d,%d,%.2f,%d\n",
            st->id, st->name, st->subject_count,
            st->scores[0], st->scores[1], st->scores[2],
            st->scores[3], st->scores[4],
            st->average, st->rank);
    }

    fclose(fp);
    return 1;
}

/* CSV 불러오기: 헤더 스킵 → 줄별 파싱 → add_student로 주입 */
int load_csv(const char* path, StudentStore* store) {
    FILE* fp;
    char  line[512];

    fp = fopen(path, "r");
    if (!fp) {
        printf("파일 열기 실패: %s\n", path);
        return 0;
    }

    /* 헤더 버림 */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }

    store->size = 0;  /* 기존 내용 초기화 */

    while (fgets(line, sizeof(line), fp)) {
        int id, sc1, sc2, sc3, sc4, sc5;
        int subject_count, rank;
        char  name[MAX_NAME_LEN];
        float avg;
        int ok = sscanf(line,
            "%d,%49[^,],%d,%d,%d,%d,%d,%d,%f,%d",
            &id, name, &subject_count,
            &sc1, &sc2, &sc3, &sc4, &sc5,
            &avg, &rank);
        if (ok == 10) {
            int scores[MAX_SUBJECTS] = { sc1, sc2, sc3, sc4, sc5 };
            add_student(store, id, name, scores, subject_count);
            /* 필요하면 avg, rank를 강제로 덮어쓸 수도 있음 */
        }
    }

    fclose(fp);
    return 1;
}
