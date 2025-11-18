/* main.c : 9주차 통합 버전
   - 6주차 기능 + 7/8/9주차 확장
   - 국어/영어/수학 같은 과목 이름은 전혀 사용하지 않고
     단순히 1~5번째 과목 점수로만 처리한다.
*/
#define _CRT_SECURE_NO_WARNINGS    /* MSVC의 fopen/scanf 경고 끄기 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "model.h"
#include "io.h"

/* 자동 백업 파일 경로 */
#define AUTOSAVE_PATH "autosave.csv"

/* 과목별 가중치, 합격 컷(7주차 기능용) */
static float g_weights[MAX_SUBJECTS] = { 1,1,1,1,1 };
static int   g_use_weights = 0;
static float g_pass_cut = 60.0f;

/* ---------------- 공통 도우미 ---------------- */

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
    puts("20) 자동 백업/되돌리기  (8주차)");
    puts("21) 성적 분포(구간별 인원 수) 보기 (9주차)");
    puts("0) 종료");
}

static void autosave_now(const StudentStore* store) {
    /* 실패해도 프로그램은 계속 진행 */
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
#if defined(_MSC_VER)
        _snprintf_s(msg, sizeof(msg), _TRUNCATE,
            "%d번째 과목 점수(0~100): ", i + 1);
#else
        snprintf(msg, sizeof(msg),
            "%d번째 과목 점수(0~100): ", i + 1);
#endif
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

/* ---------------- 2) 목록 보기(전체) ---------------- */

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
            st->average,
            grade_of_average(st->average),
            st->rank);
    }
}

/* ---------------- 3) 저장, 4) 불러오기 ---------------- */

static void action_save(StudentStore* store) {
    char path[260];
    prompt_string("저장 파일명 (예: students.csv): ", path, sizeof(path));
    if (save_csv(path, store)) puts("저장 완료.");
    else puts("저장 실패.");
}

static void action_load(StudentStore* store) {
    char path[260];
    prompt_string("불러올 파일명 (예: students.csv): ", path, sizeof(path));
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

    id = prompt_int("대상 학번: ", MIN_ID, MAX_ID);

    if (mode == 1) {
        int subject_count, i, scores[MAX_SUBJECTS];
        Student* st = find_student_by_id(store, id);
        if (!st) { puts("해당 학번 학생 없음."); return; }

        subject_count = prompt_int("새 과목 수(1~5): ", 1, MAX_SUBJECTS);
        for (i = 0; i < subject_count; ++i) {
            char msg[64];
#if defined(_MSC_VER)
            _snprintf_s(msg, sizeof(msg), _TRUNCATE,
                "%d번째 과목 점수(0~100): ", i + 1);
#else
            snprintf(msg, sizeof(msg),
                "%d번째 과목 점수(0~100): ", i + 1);
#endif
            scores[i] = prompt_int(msg, 0, 100);
        }
        for (; i < MAX_SUBJECTS; ++i) scores[i] = 0;

        if (update_student(store, id, scores, subject_count)) {
            puts("점수 수정 완료.");
            autosave_now(store);
        }
        else puts("점수 수정 실패.");
    }
    else {
        char new_name[MAX_NAME_LEN];
        if (!find_student_by_id(store, id)) {
            puts("해당 학번 학생 없음.");
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
    else puts("해당 학번 학생 없음.");
}

/* ---------------- 7) 기본 검색 ---------------- */

static void action_search_exact(StudentStore* store) {
    int mode = prompt_int("검색: 1)ID  2)이름(완전 일치) : ", 1, 2);

    if (mode == 1) {
        int id = prompt_int("학번: ", MIN_ID, MAX_ID);
        Student* st = find_student_by_id(store, id);
        if (st)
            printf("결과 → ID:%d, 이름:%s, 평균:%.2f, 등수:%d, 등급:%s\n",
                st->id, st->name, st->average, st->rank,
                grade_of_average(st->average));
        else
            puts("검색 결과 없음.");
    }
    else {
        char name[MAX_NAME_LEN];
        Student* st;
        prompt_string("이름(완전 일치): ", name, sizeof(name));
        st = find_student_by_name(store, name);
        if (st)
            printf("결과 → ID:%d, 이름:%s, 평균:%.2f, 등수:%d, 등급:%s\n",
                st->id, st->name, st->average, st->rank,
                grade_of_average(st->average));
        else
            puts("검색 결과 없음.");
    }
}

/* ---------------- 8) 간단 정렬 ---------------- */

static void action_sort_simple(StudentStore* store) {
    int mode = prompt_int("정렬: 1) 평균(내림)  2) 이름(오름) : ", 1, 2);
    if (mode == 1) {
        sort_by_average(store);
        puts("평균 기준 정렬 완료.");
    }
    else {
        sort_by_name(store);
        puts("이름 기준 정렬 완료.");
    }
    action_list(store);
}

/* ---------------- 9) 과목별 통계 ---------------- */

static void action_stats(const StudentStore* store) {
    SubjectStats stats[MAX_SUBJECTS];
    int i;

    compute_subject_stats(store, stats);

    puts("\n[과목별 통계] (과목 번호 1~5)");
    puts("과목\t참여수\t최소\t최대\t평균");
    for (i = 0; i < MAX_SUBJECTS; ++i) {
        if (stats[i].count > 0)
            printf("%d\t%d\t%d\t%d\t%.2f\n",
                i + 1, stats[i].count,
                stats[i].min, stats[i].max,
                stats[i].average);
        else
            printf("%d\t0\t-\t-\t-\n", i + 1);
    }
}

/* ---------------- 10) 고급 검색 ---------------- */

static void action_search_advanced(StudentStore* store) {
    char needle[MAX_NAME_LEN];
    Student* matches[128];
    size_t i, found;

    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    prompt_string("검색할 이름의 '일부분'(대소문자 무시): ",
        needle, sizeof(needle));

    found = find_students_by_name_substr_ci(store, needle, matches, 128);
    if (found == 0) {
        puts("검색 결과 없음.");
        return;
    }

    printf("총 %u명 찾음:\n", (unsigned)found);
    puts("ID\t이름\t평균\t등급\t등수");
    for (i = 0; i < found; ++i)
        printf("%d\t%s\t%.2f\t%s\t%d\n",
            matches[i]->id, matches[i]->name, matches[i]->average,
            grade_of_average(matches[i]->average), matches[i]->rank);
}

/* ---------------- 11) 다중 기준 정렬 ---------------- */

static void action_sort_advanced(StudentStore* store) {
    int mode;

    if (store->size < 2) {
        puts("정렬할 학생이 2명 미만입니다.");
        return;
    }

    puts("정렬 기준을 고르세요:");
    puts("1) 평균 내림 → 이름 오름");
    puts("2) 이름 오름 → ID 오름");
    puts("3) ID 오름");
    mode = prompt_int("선택: ", 1, 3);

    if (mode == 1) { sort_by_average_then_name(store); puts("정렬 완료: 평균↓, 이름↑"); }
    else if (mode == 2) { sort_by_name_then_id(store);      puts("정렬 완료: 이름↑, ID↑"); }
    else { sort_by_id(store);                puts("정렬 완료: ID↑"); }

    action_list(store);
}

/* ---------------- 12) 상위 N명 보기 ---------------- */

static void action_top_n(StudentStore* store) {
    int n, i, printed;

    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    recompute_ranks(store);
    n = prompt_int("상위 몇 명을 볼까요? N: ", 1, (int)store->size);
    sort_by_average_then_name(store);

    puts("\n[상위 N명]");
    puts("순위\tID\t이름\t평균\t등급");

    printed = 0;
    for (i = 0; i < (int)store->size && printed < n; ++i) {
        printf("%d\t%d\t%s\t%.2f\t%s\n",
            store->data[i].rank,
            store->data[i].id,
            store->data[i].name,
            store->data[i].average,
            grade_of_average(store->data[i].average));
        printed++;
    }
}

/* ---------------- 13) 학생 성적표(1명) ---------------- */

static void action_print_card(StudentStore* store) {
    int id;
    Student* st;

    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    id = prompt_int("성적표를 볼 학번: ", MIN_ID, MAX_ID);
    st = find_student_by_id(store, id);
    if (!st) {
        puts("해당 학번 학생 없음.");
        return;
    }
    print_student_pretty(st);
}

/* ---------------- 14) TXT 리포트 ---------------- */

static void action_export_report(const StudentStore* store) {
    char path[260];

    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    prompt_string("내보낼 TXT 파일명 (예: report.txt): ",
        path, sizeof(path));

    if (export_report_txt(path, store))
        puts("리포트 내보내기 완료.");
    else
        puts("리포트 내보내기 실패.");
}

/* ---------------- 15) CSV 머지 ---------------- */

static void action_import_merge(StudentStore* store) {
    char path[260];
    int overwrite;
    int added = 0, updated = 0, skipped = 0;
    int ok;

    prompt_string("머지할 CSV 파일명: ", path, sizeof(path));
    overwrite = prompt_yes_no("같은 학번이 있으면 덮어쓸까요?");

    ok = import_merge_csv(path, store, overwrite,
        &added, &updated, &skipped);
    if (!ok) {
        puts("머지 불러오기 실패.");
        return;
    }

    printf("머지 완료: 추가 %d명, 갱신 %d명, 스킵 %d명\n",
        added, updated, skipped);

    autosave_now(store);
}

/* ---------------- 16) 가중치 설정 + 평균 재계산 ---------------- */

static void recompute_weighted_averages(StudentStore* store) {
    size_t i;

    if (!g_use_weights) return;

    for (i = 0; i < store->size; ++i) {
        Student* st = &store->data[i];
        int k;
        float num = 0.0f, den = 0.0f;

        for (k = 0; k < st->subject_count; ++k) {
            float w = g_weights[k];
            num += (float)st->scores[k] * w;
            den += w;
        }
        if (den > 0.0f)
            st->average = num / den;
        else
            st->average = 0.0f;
    }

    recompute_ranks(store);
}

static void action_set_weights(StudentStore* store) {
    int use = prompt_yes_no("과목별 가중치를 사용할까요? (Y=사용, N=사용 안 함)");

    if (!use) {
        int i;
        for (i = 0; i < MAX_SUBJECTS; ++i) g_weights[i] = 1.0f;
        g_use_weights = 0;
        puts("가중치 사용을 끕니다. 기본 평균을 사용합니다.");
        /* 평균은 add/update 시 계산된 값 그대로 둔다. */
        return;
    }

    {
        int i;
        g_use_weights = 1;
        for (i = 0; i < MAX_SUBJECTS; ++i) {
            char msg[64];
#if defined(_MSC_VER)
            _snprintf_s(msg, sizeof(msg), _TRUNCATE,
                "%d번째 과목 가중치(0~100): ", i + 1);
#else
            snprintf(msg, sizeof(msg),
                "%d번째 과목 가중치(0~100): ", i + 1);
#endif
            g_weights[i] = (float)prompt_int(msg, 0, 100);
        }
    }

    recompute_weighted_averages(store);
    autosave_now(store);
    puts("가중치를 적용해 평균/등수를 다시 계산했습니다.");
}

/* ---------------- 17) 합격 기준(컷) 설정 ---------------- */

static void action_set_pass_cut(void) {
    int cut = prompt_int("합격 기준 평균 점수(0~100): ", 0, 100);
    g_pass_cut = (float)cut;
    printf("합격 기준을 %.1f점으로 설정했습니다.\n", g_pass_cut);
}

/* ---------------- 18) JSON 내보내기 ---------------- */

static void action_export_json(const StudentStore* store) {
    char path[260];
    size_t i, j;

    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    prompt_string("내보낼 JSON 파일명 (예: students.json): ",
        path, sizeof(path));

    FILE* fp = fopen(path, "w");
    if (!fp) {
        printf("파일 열기 실패: %s\n", path);
        return;
    }

    fprintf(fp, "[\n");
    for (i = 0; i < store->size; ++i) {
        const Student* st = &store->data[i];
        fprintf(fp, "  {\n");
        fprintf(fp, "    \"id\": %d,\n", st->id);
        fprintf(fp, "    \"name\": \"%s\",\n", st->name);
        fprintf(fp, "    \"subject_count\": %d,\n", st->subject_count);
        fprintf(fp, "    \"scores\": [");
        for (j = 0; j < (size_t)st->subject_count; ++j) {
            fprintf(fp, "%d", st->scores[j]);
            if (j + 1 < (size_t)st->subject_count) fprintf(fp, ", ");
        }
        fprintf(fp, "],\n");
        fprintf(fp, "    \"average\": %.2f,\n", st->average);
        fprintf(fp, "    \"rank\": %d\n", st->rank);
        fprintf(fp, "  }%s\n", (i + 1 < store->size) ? "," : "");
    }
    fprintf(fp, "]\n");

    fclose(fp);
    puts("JSON 내보내기 완료.");
}

/* ---------------- 19) 페이지 목록 ---------------- */

static void action_list_paged(const StudentStore* store) {
    const int per_page = 5;
    int total = (int)store->size;
    int page = 0;
    char cmd[16];

    if (total == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    while (1) {
        int start = page * per_page;
        int end = start + per_page;
        int page_count;
        size_t i, j;

        if (start >= total) { page = 0; start = 0; end = per_page; }
        if (end > total)    end = total;

        page_count = (total + per_page - 1) / per_page;

        printf("\n[학생 목록 - 페이지 %d / %d]\n",
            page + 1, page_count);
        puts("ID\t이름\t과목수\t점수들\t\t평균\t등급\t등수");
        for (i = (size_t)start; i < (size_t)end; ++i) {
            const Student* st = &store->data[i];
            printf("%d\t%s\t%d\t", st->id, st->name, st->subject_count);
            for (j = 0; j < (size_t)st->subject_count; ++j)
                printf("%d ", st->scores[j]);
            if (st->subject_count < MAX_SUBJECTS) printf("\t");
            printf("\t%.2f\t%s\t%d\n",
                st->average,
                grade_of_average(st->average),
                st->rank);
        }

        printf("\nN:다음 P:이전 Q:종료 입력: ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        if (cmd[0] == 'N' || cmd[0] == 'n') {
            if ((page + 1) * per_page >= total)
                puts("마지막 페이지입니다.");
            else page++;
        }
        else if (cmd[0] == 'P' || cmd[0] == 'p') {
            if (page == 0)
                puts("첫 페이지입니다.");
            else page--;
        }
        else if (cmd[0] == 'Q' || cmd[0] == 'q') {
            break;
        }
        else {
            puts("N, P, Q 중 하나를 입력하세요.");
        }
    }
}

/* ---------------- 20) 자동 백업/되돌리기 ---------------- */

static void action_restore_from_autosave(StudentStore* store) {
    puts("자동 백업 파일에서 되돌리기(autosave.csv 사용).");
    if (!prompt_yes_no("정말 되돌릴까요? 현재 내용은 사라집니다."))
        return;

    if (load_csv(AUTOSAVE_PATH, store))
        puts("자동 백업에서 되돌리기 완료.");
    else
        puts("되돌리기 실패(백업 파일이 없거나 손상됨).");
}

/* ---------------- 21) 성적 분포 ---------------- */

static void action_score_distribution(const StudentStore* store) {
    int bins[5] = { 0,0,0,0,0 };
    int pass_cnt = 0, fail_cnt = 0;
    size_t i;

    if (store->size == 0) {
        puts("등록된 학생이 없습니다.");
        return;
    }

    for (i = 0; i < store->size; ++i) {
        float avg = store->data[i].average;

        if (avg < 60.0f) bins[0]++;
        else if (avg < 70.0f) bins[1]++;
        else if (avg < 80.0f) bins[2]++;
        else if (avg < 90.0f) bins[3]++;
        else                  bins[4]++;

        if (avg >= g_pass_cut) pass_cnt++;
        else                   fail_cnt++;
    }

    puts("\n[성적 분포(평균 기준)]");
    printf("0~59  점: %d명\n", bins[0]);
    printf("60~69 점: %d명\n", bins[1]);
    printf("70~79 점: %d명\n", bins[2]);
    printf("80~89 점: %d명\n", bins[3]);
    printf("90~100점: %d명\n", bins[4]);

    printf("\n현재 합격 기준: %.1f점\n", g_pass_cut);
    printf("합격: %d명, 불합격: %d명\n", pass_cnt, fail_cnt);
}

/* ---------------- main ---------------- */

int main(void) {
    StudentStore store;
    int running = 1;

    init_store(&store);

    while (running) {
        int cmd;
        print_menu();
        cmd = prompt_int("메뉴 선택: ", 0, 21);

        switch (cmd) {
        case 0:  running = 0;                              break;
        case 1:  action_add(&store);                       break;
        case 2:  action_list(&store);                      break;
        case 3:  action_save(&store);                      break;
        case 4:  action_load(&store);                      break;
        case 5:  action_update(&store);                    break;
        case 6:  action_delete(&store);                    break;
        case 7:  action_search_exact(&store);              break;
        case 8:  action_sort_simple(&store);               break;
        case 9:  action_stats(&store);                     break;
        case 10: action_search_advanced(&store);           break;
        case 11: action_sort_advanced(&store);             break;
        case 12: action_top_n(&store);                     break;
        case 13: action_print_card(&store);                break;
        case 14: action_export_report(&store);             break;
        case 15: action_import_merge(&store);              break;
        case 16: action_set_weights(&store);               break;
        case 17: action_set_pass_cut();                    break;
        case 18: action_export_json(&store);               break;
        case 19: action_list_paged(&store);                break;
        case 20: action_restore_from_autosave(&store);     break;
        case 21: action_score_distribution(&store);        break;
        default: puts("잘못된 선택입니다.");                break;
        }
    }

    free_store(&store);
    puts("프로그램을 종료합니다.");
    return EXIT_SUCCESS;
}
