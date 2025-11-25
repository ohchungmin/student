/* main.c : 10주차 통합 버전
   - 1~9주차 모든 기능 포함
   - 10주차 신규 기능(22, 23번 메뉴) 추가
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "model.h"
#include "io.h"

#define AUTOSAVE_PATH "autosave.csv"

/* 과목 가중치, 합격 컷 */
static float g_weights[MAX_SUBJECTS] = { 1,1,1,1,1 };
static int   g_use_weights = 0;
static float g_pass_cut = 60.0f;

/* ---------------- 메뉴 출력 ---------------- */

static void print_menu(void) {
    puts("\n==== 학생 성적 관리 ====");
    puts("1) 학생 추가");
    puts("2) 학생 목록 보기");
    puts("3) 저장 (CSV)");
    puts("4) 불러오기 (CSV)");
    puts("5) 학생 점수/이름 수정");
    puts("6) 학생 삭제");
    puts("7) 학생 검색(정확 일치)");
    puts("8) 정렬(평균/이름)");
    puts("9) 과목별 통계");
    puts("10) 고급 검색(부분/대소문자 무시)");
    puts("11) 다중 기준 정렬");
    puts("12) 상위 N명 보기");
    puts("13) 학생 성적표(1명) 출력");
    puts("14) 전체 리포트 내보내기(TXT)");
    puts("15) CSV 머지 불러오기(덮어쓰기 선택)");
    puts("16) 과목별 가중치 설정 + 평균 재계산 (7주차)");
    puts("17) 합격 기준(컷) 설정 (7주차)");
    puts("18) JSON 내보내기 (7주차)");
    puts("19) 페이지로 목록 보기 (7주차)");
    puts("20) 자동 백업/되돌리기 (8주차)");
    puts("21) 성적 분포(구간별 인원 수) 보기 (9주차)");
    puts("22) 성적 편차 분석 (10주차)");
    puts("23) 성적 위험군 탐지 (10주차)");
    puts("0) 종료");
}

/* 자동 백업 */
static void autosave_now(const StudentStore* store) {
    save_csv(AUTOSAVE_PATH, store);
}

/* ---------------- 1) 학생 추가 ---------------- */

static void action_add(StudentStore* store) {
    int id, subject_count, i;
    char name[MAX_NAME_LEN];
    int scores[MAX_SUBJECTS];

    id = prompt_int("학번(ID): ", MIN_ID, MAX_ID);
    prompt_string("이름: ", name, sizeof(name));
    subject_count = prompt_int("과목 수(1~5): ", 1, MAX_SUBJECTS);

    for (i = 0; i < subject_count; ++i) {
        char msg[64];
        snprintf(msg, sizeof(msg), "%d번째 과목 점수(0~100): ", i + 1);
        scores[i] = prompt_int(msg, 0, 100);
    }
    for (; i < MAX_SUBJECTS; ++i) scores[i] = 0;

    if (add_student(store, id, name, scores, subject_count)) {
        puts("학생이 추가되었습니다.");
        autosave_now(store);
    }
    else {
        puts("추가 실패(중복 학번이거나 입력 오류).");
    }
}

/* ---------------- 2) 학생 목록 ---------------- */

static void action_list(const StudentStore* store) {
    size_t i, j;
    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    puts("\n[학생 목록]");
    puts("ID\t이름\t과목수\t점수들\t\t평균\t등급\t등수");
    for (i = 0; i < store->size; ++i) {
        const Student* st = &store->data[i];
        printf("%d\t%s\t%d\t", st->id, st->name, st->subject_count);

        for (j = 0; j < (size_t)st->subject_count; ++j)
            printf("%d ", st->scores[j]);
        if (st->subject_count < MAX_SUBJECTS) printf("\t");

        printf("\t%.2f\t%s\t%d\n",
            st->average, grade_of_average(st->average), st->rank);
    }
}

/* ---------------- 3) CSV 저장 ---------------- */

static void action_save(StudentStore* store) {
    char path[260];
    prompt_string("저장 파일명: ", path, sizeof(path));

    if (save_csv(path, store)) puts("저장 완료.");
    else puts("저장 실패.");
}

/* ---------------- 4) CSV 불러오기 ---------------- */

static void action_load(StudentStore* store) {
    char path[260];
    prompt_string("불러올 파일명: ", path, sizeof(path));

    if (load_csv(path, store)) {
        puts("불러오기 완료.");
        autosave_now(store);
    }
    else puts("불러오기 실패.");
}

/* ---------------- 5) 수정 ---------------- */

static void action_update(StudentStore* store) {
    int mode, id;
    puts("무엇을 수정할까요?");
    puts("1) 점수 전체 다시 입력");
    puts("2) 이름만 수정");
    mode = prompt_int("선택: ", 1, 2);
    id = prompt_int("학번: ", MIN_ID, MAX_ID);

    if (mode == 1) {
        Student* st = find_student_by_id(store, id);
        if (!st) { puts("해당 학번 없음."); return; }

        int subject_count = prompt_int("새 과목 수: ", 1, MAX_SUBJECTS);
        int scores[MAX_SUBJECTS], i;

        for (i = 0; i < subject_count; ++i) {
            char msg[64];
            snprintf(msg, sizeof(msg), "%d번째 과목 점수: ", i + 1);
            scores[i] = prompt_int(msg, 0, 100);
        }
        for (; i < MAX_SUBJECTS; ++i) scores[i] = 0;

        if (update_student(store, id, scores, subject_count)) {
            puts("수정 완료.");
            autosave_now(store);
        }
        else puts("수정 실패.");

    }
    else {
        char new_name[MAX_NAME_LEN];
        if (!find_student_by_id(store, id)) {
            puts("해당 학번 없음.");
            return;
        }
        prompt_string("새 이름: ", new_name, sizeof(new_name));
        if (update_student_name(store, id, new_name)) {
            puts("이름 수정 완료.");
            autosave_now(store);
        }
        else puts("이름 수정 실패.");
    }
}

/* ---------------- 6) 삭제 ---------------- */

static void action_delete(StudentStore* store) {
    int id = prompt_int("삭제할 학번: ", MIN_ID, MAX_ID);
    if (delete_student(store, id)) {
        puts("삭제 완료.");
        autosave_now(store);
    }
    else puts("해당 학번 없음.");
}

/* ---------------- 7) 정확 검색 ---------------- */

static void action_search_exact(StudentStore* store) {
    int mode = prompt_int("검색: 1)ID 2)이름 : ", 1, 2);

    if (mode == 1) {
        int id = prompt_int("학번: ", MIN_ID, MAX_ID);
        Student* st = find_student_by_id(store, id);
        if (st) {
            printf("ID:%d, 이름:%s, 평균:%.2f, 등급:%s, 등수:%d\n",
                st->id, st->name, st->average,
                grade_of_average(st->average), st->rank);
        }
        else puts("검색 결과 없음.");
    }
    else {
        char name[MAX_NAME_LEN];
        prompt_string("이름: ", name, sizeof(name));
        Student* st = find_student_by_name(store, name);
        if (st) {
            printf("ID:%d, 이름:%s, 평균:%.2f, 등급:%s, 등수:%d\n",
                st->id, st->name, st->average,
                grade_of_average(st->average), st->rank);
        }
        else puts("검색 결과 없음.");
    }
}

/* ---------------- 8) 단순 정렬 ---------------- */

static void action_sort_simple(StudentStore* store) {
    int mode = prompt_int("정렬: 1)평균(내림) 2)이름(오름): ", 1, 2);
    if (mode == 1) sort_by_average(store);
    else sort_by_name(store);

    action_list(store);
}

/* ---------------- 9) 과목별 통계 ---------------- */

static void action_stats(const StudentStore* store) {
    SubjectStats stats[MAX_SUBJECTS];
    compute_subject_stats(store, stats);

    puts("[과목별 통계]");
    puts("과목\t참여수\t최소\t최대\t평균");
    for (int i = 0; i < MAX_SUBJECTS; ++i) {
        if (stats[i].count > 0)
            printf("%d\t%d\t%d\t%d\t%.2f\n",
                i + 1, stats[i].count,
                stats[i].min, stats[i].max, stats[i].average);
        else
            printf("%d\t0\t-\t-\t-\n", i + 1);
    }
}

/* ---------------- 10) 고급 검색 ---------------- */

static void action_search_advanced(StudentStore* store) {
    char key[MAX_NAME_LEN];
    Student* matches[128];
    size_t found;

    prompt_string("검색 키워드: ", key, sizeof(key));
    found = find_students_by_name_substr_ci(store, key, matches, 128);

    if (found == 0) {
        puts("검색 결과 없음.");
        return;
    }

    puts("ID\t이름\t평균\t등급\t등수");
    for (size_t i = 0; i < found; ++i) {
        Student* st = matches[i];
        printf("%d\t%s\t%.2f\t%s\t%d\n",
            st->id, st->name, st->average,
            grade_of_average(st->average), st->rank);
    }
}

/* ---------------- 11) 다중 정렬 ---------------- */

static void action_sort_advanced(StudentStore* store) {
    int mode = prompt_int(
        "1) 평균↓,이름↑  2) 이름↑,ID↑  3) ID↑ : ", 1, 3);

    if (mode == 1) sort_by_average_then_name(store);
    else if (mode == 2) sort_by_name_then_id(store);
    else sort_by_id(store);

    action_list(store);
}

/* ---------------- 12) 상위 N명 ---------------- */

static void action_top_n(StudentStore* store) {
    recompute_ranks(store);
    int n = prompt_int("N명: ", 1, (int)store->size);

    sort_by_average_then_name(store);
    puts("순위\tID\t이름\t평균\t등급");

    int count = 0;
    for (size_t i = 0; i < store->size && count < n; ++i) {
        Student* st = &store->data[i];
        printf("%d\t%d\t%s\t%.2f\t%s\n",
            st->rank, st->id, st->name,
            st->average, grade_of_average(st->average));
        count++;
    }
}

/* ---------------- 13) 성적표 ---------------- */

static void action_print_card(StudentStore* store) {
    int id = prompt_int("학번: ", MIN_ID, MAX_ID);
    Student* st = find_student_by_id(store, id);

    if (!st) {
        puts("해당 학번 없음.");
        return;
    }
    print_student_pretty(st);
}

/* ---------------- 14) TXT 리포트 ---------------- */

static void action_export_report(const StudentStore* store) {
    char path[260];
    prompt_string("TXT 파일명: ", path, sizeof(path));

    if (export_report_txt(path, store))
        puts("완료.");
    else
        puts("실패.");
}

/* ---------------- 15) CSV 머지 ---------------- */

static void action_import_merge(StudentStore* store) {
    char path[260];
    prompt_string("CSV 파일명: ", path, sizeof(path));

    int overwrite = prompt_yes_no("덮어쓸까요?");
    int added = 0, updated = 0, skipped = 0;

    if (!import_merge_csv(path, store, overwrite,
        &added, &updated, &skipped)) {
        puts("머지 실패.");
        return;
    }
    printf("추가:%d 갱신:%d 스킵:%d\n", added, updated, skipped);
    autosave_now(store);
}

/* ---------------- 16) 가중치 설정 ---------------- */

static void recompute_weighted_averages(StudentStore* store) {
    if (!g_use_weights) return;

    for (size_t i = 0; i < store->size; ++i) {
        float num = 0, den = 0;
        for (int k = 0; k < store->data[i].subject_count; ++k) {
            num += store->data[i].scores[k] * g_weights[k];
            den += g_weights[k];
        }
        store->data[i].average = (den > 0 ? num / den : 0.0f);
    }
    recompute_ranks(store);
}

static void action_set_weights(StudentStore* store) {
    int use = prompt_yes_no("가중치 사용(Y)? ");
    if (!use) {
        g_use_weights = 0;
        puts("가중치 해제.");
        return;
    }
    g_use_weights = 1;
    for (int i = 0; i < MAX_SUBJECTS; ++i) {
        char msg[64];
        snprintf(msg, sizeof(msg),
            "%d번째 과목 가중치(0~100): ", i + 1);
        g_weights[i] = (float)prompt_int(msg, 0, 100);
    }
    recompute_weighted_averages(store);
    autosave_now(store);
    puts("가중치 적용됨.");
}

/* ---------------- 17) 합격 컷 ---------------- */

static void action_set_pass_cut(void) {
    int cut = prompt_int("합격 평균 기준: ", 0, 100);
    g_pass_cut = (float)cut;
}

/* ---------------- 18) JSON 내보내기 ---------------- */

static void action_export_json(const StudentStore* store) {
    char path[260];
    prompt_string("JSON 파일명: ", path, sizeof(path));

    FILE* fp = fopen(path, "w");
    if (!fp) {
        puts("파일 생성 실패.");
        return;
    }

    fprintf(fp, "[\n");
    for (size_t i = 0; i < store->size; ++i) {
        const Student* st = &store->data[i];
        fprintf(fp,
            "  {\"id\":%d, \"name\":\"%s\", \"subject_count\":%d, \"scores\":[",
            st->id, st->name, st->subject_count);

        for (int j = 0; j < st->subject_count; ++j) {
            fprintf(fp, "%d%s",
                st->scores[j],
                (j + 1 < st->subject_count ? ", " : ""));
        }

        fprintf(fp,
            "], \"average\":%.2f, \"rank\":%d}%s\n",
            st->average, st->rank,
            (i + 1 < store->size) ? "," : "");
    }
    fprintf(fp, "]\n");
    fclose(fp);
    puts("JSON 내보내기 완료.");
}

/* ---------------- 19) 페이지 출력 ---------------- */

static void action_list_paged(const StudentStore* store) {
    const int per = 5;
    int total = (int)store->size;
    int page = 0;
    char cmd[16];

    if (total == 0) {
        puts("학생 없음.");
        return;
    }

    while (1) {
        int start = page * per;
        int end = start + per;
        if (end > total) end = total;

        printf("\n[페이지 %d]\n", page + 1);
        puts("ID\t이름\t평균\t등급\t등수");

        for (int i = start; i < end; ++i) {
            const Student* st = &store->data[i];
            printf("%d\t%s\t%.2f\t%s\t%d\n",
                st->id, st->name, st->average,
                grade_of_average(st->average), st->rank);
        }

        printf("\nN:다음 P:이전 Q:종료 -> ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        if (cmd[0] == 'N' || cmd[0] == 'n') page++;
        else if (cmd[0] == 'P' || cmd[0] == 'p') { if (page > 0) page--; }
        else if (cmd[0] == 'Q' || cmd[0] == 'q') break;
    }
}

/* ---------------- 20) 자동 백업 복구 ---------------- */

static void action_restore_from_autosave(StudentStore* store) {
    if (!prompt_yes_no("되돌릴까요? 현재 내용 삭제됨!")) return;

    if (load_csv(AUTOSAVE_PATH, store))
        puts("복구 완료.");
    else
        puts("복구 실패.");
}

/* ---------------- 21) 성적 분포 ---------------- */

static void action_score_distribution(const StudentStore* store) {
    int bins[5] = { 0 }, pass = 0, fail = 0;

    puts("[성적 분포]");
    for (size_t i = 0; i < store->size; ++i) {
        float a = store->data[i].average;

        if (a < 60) bins[0]++;
        else if (a < 70) bins[1]++;
        else if (a < 80) bins[2]++;
        else if (a < 90) bins[3]++;
        else bins[4]++;

        if (a >= g_pass_cut) pass++;
        else fail++;
    }

    printf("0~59 : %d명\n", bins[0]);
    printf("60~69: %d명\n", bins[1]);
    printf("70~79: %d명\n", bins[2]);
    printf("80~89: %d명\n", bins[3]);
    printf("90~100: %d명\n", bins[4]);

    printf("합격:%d명 / 불합격:%d명\n", pass, fail);
}

/* ===================== 10주차 신규 기능 ===================== */

/* ---------------- 22) 성적 편차 분석 ---------------- */

static void action_deviation_analysis(const StudentStore* store) {
    if (store->size == 0) {
        puts("학생 없음.");
        return;
    }

    float sum = 0;
    for (size_t i = 0; i < store->size; ++i)
        sum += store->data[i].average;

    float overall = sum / store->size;

    printf("\n[성적 편차 분석] 전체 평균: %.2f\n", overall);
    puts("ID\t이름\t평균\t편차");

    for (size_t i = 0; i < store->size; ++i) {
        float dev = store->data[i].average - overall;
        printf("%d\t%s\t%.2f\t%+.2f\n",
            store->data[i].id,
            store->data[i].name,
            store->data[i].average,
            dev);
    }
}

/* ---------------- 23) 성적 위험군 탐지 ---------------- */

static void action_risk_student(const StudentStore* store) {
    puts("\n[성적 위험군 탐지] (평균 < 60)");

    int count = 0;

    puts("ID\t이름\t평균\t등급\t등수");
    for (size_t i = 0; i < store->size; ++i) {
        const Student* st = &store->data[i];
        if (st->average < 60.0f) {
            printf("%d\t%s\t%.2f\t%s\t%d\n",
                st->id, st->name, st->average,
                grade_of_average(st->average), st->rank);
            count++;
        }
    }

    printf("\n총 %d명이 위험군입니다.\n", count);
}

/* ========================= main ========================= */

int main(void) {
    StudentStore store;
    int running = 1;

    init_store(&store);

    while (running) {
        print_menu();
        int cmd = prompt_int("메뉴 선택: ", 0, 23);

        switch (cmd) {
        case 0: running = 0; break;
        case 1: action_add(&store); break;
        case 2: action_list(&store); break;
        case 3: action_save(&store); break;
        case 4: action_load(&store); break;
        case 5: action_update(&store); break;
        case 6: action_delete(&store); break;
        case 7: action_search_exact(&store); break;
        case 8: action_sort_simple(&store); break;
        case 9: action_stats(&store); break;
        case 10: action_search_advanced(&store); break;
        case 11: action_sort_advanced(&store); break;
        case 12: action_top_n(&store); break;
        case 13: action_print_card(&store); break;
        case 14: action_export_report(&store); break;
        case 15: action_import_merge(&store); break;
        case 16: action_set_weights(&store); break;
        case 17: action_set_pass_cut(); break;
        case 18: action_export_json(&store); break;
        case 19: action_list_paged(&store); break;
        case 20: action_restore_from_autosave(&store); break;
        case 21: action_score_distribution(&store); break;
        case 22: action_deviation_analysis(&store); break;
        case 23: action_risk_student(&store); break;

        default: puts("잘못된 선택."); break;
        }
    }

    free_store(&store);
    puts("프로그램 종료.");
    return EXIT_SUCCESS;
}
