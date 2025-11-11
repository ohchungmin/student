// io.c : CSV/JSON/TXT 구현
#define _CRT_SECURE_NO_WARNINGS
#include "io.h"
#include <stdio.h>
#include <string.h>

int save_to_csv(const StudentStore* s, const char* path) {
    FILE* fp = fopen(path, "w");
    if (!fp) { printf("파일 열기 실패: %s\n", path); return -1; }
    fprintf(fp, "id,name,kor,eng,math,deleted\n");
    for (int i = 0;i < s->count;i++) {
        const Student* p = &s->arr[i];
        fprintf(fp, "%d,%s,%d,%d,%d,%d\n", p->id, p->name, p->kor, p->eng, p->math, p->deleted);
    }
    fclose(fp);
    return 0;
}

int load_from_csv(StudentStore* s, const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) { return 0; } // 파일 없으면 0으로 간주
    // 초기화 후 읽기
    s->count = 0;
    char line[256];
    // 헤더 스킵
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return 0; }
    while (fgets(line, sizeof(line), fp)) {
        Student x;
        int id, kor, eng, math, del;
        char name[NAME_LEN];
        if (sscanf(line, "%d,%31[^,],%d,%d,%d,%d", &id, name, &kor, &eng, &math, &del) == 6) {
            x.id = id; strncpy(x.name, name, NAME_LEN - 1); x.name[NAME_LEN - 1] = '\0';
            x.kor = kor; x.eng = eng; x.math = math; x.deleted = del;
            // 총점/평균/학점은 가중치 재계산 필요. 여기선 임시로 1/3 가중치로 계산,
            // 호출 측에서 store_recalc_all()을 한번 더 호출해 최종 보정.
            x.total = x.kor + x.eng + x.math;
            x.avg = (x.kor + x.eng + x.math) / 3.0;
            x.grade = calc_grade_from_avg(x.avg);
            if (s->count < MAX_STU) s->arr[s->count++] = x;
        }
    }
    fclose(fp);
    return s->count;
}

int export_to_json(const StudentStore* s, const char* path) {
    FILE* fp = fopen(path, "w");
    if (!fp) { printf("JSON 저장 실패: %s\n", path); return -1; }
    fprintf(fp, "[\n");
    int wrote = 0;
    for (int i = 0;i < s->count;i++) {
        const Student* p = &s->arr[i];
        fprintf(fp, "  {\"id\":%d,\"name\":\"%s\",\"kor\":%d,\"eng\":%d,\"math\":%d,"
            "\"total\":%d,\"avg\":%.2f,\"grade\":\"%c\",\"deleted\":%d}%s\n",
            p->id, p->name, p->kor, p->eng, p->math, p->total, p->avg, p->grade, p->deleted,
            (i == s->count - 1) ? "" : ",");
        wrote++;
    }
    fprintf(fp, "]\n");
    fclose(fp);
    return wrote;
}

int export_all_reports_txt(const StudentStore* s, const char* path) {
    FILE* fp = fopen(path, "w");
    if (!fp) { printf("TXT 저장 실패: %s\n", path); return -1; }
    for (int i = 0;i < s->count;i++) {
        const Student* p = &s->arr[i];
        if (p->deleted) continue;
        fprintf(fp, "==== 성적표 ====\n");
        fprintf(fp, "학번: %d\n이름: %s\n", p->id, p->name);
        fprintf(fp, "국어: %d, 영어: %d, 수학: %d\n", p->kor, p->eng, p->math);
        fprintf(fp, "총점: %d, 평균: %.2f, 학점: %c\n\n", p->total, p->avg, p->grade);
    }
    fclose(fp);
    return 0;
}

int merge_from_csv(StudentStore* s, const char* path, int mode) {
    StudentStore tmp; store_init(&tmp);
    if (load_from_csv(&tmp, path) <= 0) return -1;
    store_recalc_all(&tmp);

    if (mode == 0) {
        // 덮어쓰기: 통째로 교체
        *s = tmp;
        return s->count;
    }
    // 병합: 같은 학번은 업데이트, 없으면 추가
    int changed = 0;
    for (int i = 0;i < tmp.count;i++) {
        Student* p = &tmp.arr[i];
        int idx = store_find_index_by_id(s, p->id);
        if (idx < 0) { if (store_add(s, *p)) changed++; }
        else {
            s->arr[idx] = *p; changed++;
        }
    }
    return changed;
}
