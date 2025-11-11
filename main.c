#include <stdio.h>
#include <stdlib.h>
#include "model.h"
#include "io.h"

/* 7주차: 전역 설정 인스턴스 */
static GradeConfig g_cfg;

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
    puts("0) 종료");
}

/* ========== 액션들(기존 요약) ========== */
static void action_add(StudentStore* s) {
    int id = prompt_int("학번: ", MIN_ID, MAX_ID);
    char name[MAX_NAME_LEN]; prompt_string("이름: ", name, sizeof(name));
    int n = prompt_int("과목 수(1~5): ", 1, MAX_SUBJECTS);
    int i, scores[MAX_SUBJECTS]; for (i = 0;i < n;++i) {
        char m[64];
#if defined(_MSC_VER)
        _snprintf_s(m, sizeof(m), _TRUNCATE, "%d번째 점수(0~100): ", i + 1);
#else
        snprintf(m, sizeof(m), "%d번째 점수(0~100): ", i + 1);
#endif
        scores[i] = prompt_int(m, 0, 100);
    }
    for (;i < MAX_SUBJECTS;++i) scores[i] = 0;
    if (add_student(s, id, name, scores, n)) { puts("추가 완료."); /* 가중치 반영 */ recompute_all_averages(s, &g_cfg); }
    else puts("추가 실패.");
}

static void action_list(const StudentStore* s) {
    size_t i, j; if (s->size == 0) { puts("학생 없음."); return; }
    puts("ID\t이름\t과목수\t점수들\t\t평균\t등급\t합격\t등수");
    for (i = 0;i < s->size;++i) {
        const Student* st = &s->data[i];
        printf("%d\t%s\t%d\t", st->id, st->name, st->subject_count);
        for (j = 0;j < (size_t)st->subject_count;++j) printf("%d ", st->scores[j]);
        printf("\t%.2f\t%s\t%s\t%d\n", st->average, grade_of_average(st->average),
            pass_label(st->average, g_cfg.pass_threshold), st->rank);
    }
}

static void action_save(StudentStore* s) {
    char p[260]; prompt_string("CSV 파일명: ", p, sizeof(p));
    if (save_csv(p, s)) puts("저장 완료."); else puts("저장 실패.");
}
static void action_load(StudentStore* s) {
    char p[260]; prompt_string("불러올 CSV: ", p, sizeof(p));
    if (load_csv(p, s)) { puts("불러오기 완료."); recompute_all_averages(s, &g_cfg); }
    else puts("불러오기 실패.");
}

/* 기존: 수정/삭제/검색/정렬/통계/TopN/성적표/리포트/머지 (이하 일부만 재사용) */
static void action_update(StudentStore* s) {
    int mode = prompt_int("1) 점수  2) 이름 : ", 1, 2);
    int id = prompt_int("대상 학번: ", MIN_ID, MAX_ID);
    if (mode == 1) {
        int n = prompt_int("새 과목 수(1~5): ", 1, MAX_SUBJECTS);
        int i, sc[MAX_SUBJECTS]; for (i = 0;i < n;++i) {
            char m[64];
#if defined(_MSC_VER)
            _snprintf_s(m, sizeof(m), _TRUNCATE, "%d번째 점수(0~100): ", i + 1);
#else
            snprintf(m, sizeof(m), "%d번째 점수(0~100): ", i + 1);
#endif
            sc[i] = prompt_int(m, 0, 100);
        }
        for (;i < MAX_SUBJECTS;++i) sc[i] = 0;
        if (update_student(s, id, sc, n)) { puts("점수 수정 완료."); recompute_all_averages(s, &g_cfg); }
        else puts("수정 실패.");
    }
    else {
        char nm[MAX_NAME_LEN]; prompt_string("새 이름: ", nm, sizeof(nm));
        if (update_student_name(s, id, nm)) puts("이름 수정 완료."); else puts("수정 실패.");
    }
}
static void action_delete(StudentStore* s) {
    int id = prompt_int("삭제 학번: ", MIN_ID, MAX_ID);
    if (delete_student(s, id)) { puts("삭제 완료."); recompute_ranks(s); }
    else puts("해당 학번 없음.");
}
static void action_search_exact(StudentStore* s) {
    int m = prompt_int("1)ID  2)이름(완전일치): ", 1, 2);
    if (m == 1) {
        int id = prompt_int("학번: ", MIN_ID, MAX_ID); Student* st = find_student_by_id(s, id);
        if (st) printf("ID:%d 이름:%s 평균:%.2f 등급:%s 합격:%s 등수:%d\n",
            st->id, st->name, st->average, grade_of_average(st->average),
            pass_label(st->average, g_cfg.pass_threshold), st->rank);
        else puts("없음.");
    }
    else {
        char nm[MAX_NAME_LEN]; prompt_string("이름: ", nm, sizeof(nm)); Student* st = find_student_by_name(s, nm);
        if (st) printf("ID:%d 이름:%s 평균:%.2f 등급:%s 합격:%s 등수:%d\n",
            st->id, st->name, st->average, grade_of_average(st->average),
            pass_label(st->average, g_cfg.pass_threshold), st->rank);
        else puts("없음.");
    }
}
static void action_sort_simple(StudentStore* s) {
    int m = prompt_int("1)평균(내림)  2)이름(오름): ", 1, 2);
    if (m == 1) sort_by_average(s); else sort_by_name(s);
    recompute_ranks(s); action_list(s);
}
static void action_sort_advanced(StudentStore* s) {
    int m; if (s->size < 2) { puts("2명 이상 필요."); return; }
    puts("1) 평균↓, 이름↑  2) 이름↑, ID↑  3) ID↑");
    m = prompt_int("선택: ", 1, 3);
    if (m == 1) sort_by_average_then_name(s);
    else if (m == 2) sort_by_name_then_id(s);
    else sort_by_id(s);
    recompute_ranks(s); action_list(s);
}
static void action_stats(const StudentStore* s) {
    SubjectStats st[MAX_SUBJECTS]; int i; compute_subject_stats(s, st);
    puts("과목\t참여수\t최소\t최대\t평균");
    for (i = 0;i < MAX_SUBJECTS;++i) {
        if (st[i].count) printf("%d\t%d\t%d\t%d\t%.2f\n", i + 1, st[i].count, st[i].min, st[i].max, st[i].average);
        else printf("%d\t0\t-\t-\t-\n", i + 1);
    }
}
static void action_top_n(StudentStore* s) {
    if (s->size == 0) { puts("학생 없음."); return; }
    recompute_ranks(s);
    sort_by_average_then_name(s);
    { int n = prompt_int("상위 N: ", 1, (int)s->size), i;
    puts("순위\tID\t이름\t평균\t등급\t합격");
    for (i = 0;i < n && i < (int)s->size;++i) {
        printf("%d\t%d\t%s\t%.2f\t%s\t%s\n", s->data[i].rank, s->data[i].id, s->data[i].name,
            s->data[i].average, grade_of_average(s->data[i].average),
            pass_label(s->data[i].average, g_cfg.pass_threshold));
    } }
}
static void action_print_card(StudentStore* s) {
    int id = prompt_int("성적표 학번: ", MIN_ID, MAX_ID); Student* st = find_student_by_id(s, id);
    if (!st) { puts("없음."); return; } print_student_pretty(st);
}
static void action_export_report(const StudentStore* s) {
    char p[260]; prompt_string("TXT 파일명: ", p, sizeof(p));
    if (export_report_txt(p, s)) puts("완료."); else puts("실패.");
}
static void action_import_merge(StudentStore* s) {
    char p[260]; int ow = prompt_yes_no("같은 학번 덮어쓸까요?");
    int a = 0, u = 0, k = 0; prompt_string("머지 CSV: ", p, sizeof(p));
    if (import_merge_csv(p, s, ow, &a, &u, &k)) { printf("추가 %d, 갱신 %d, 스킵 %d\n", a, u, k); recompute_all_averages(s, &g_cfg); }
    else puts("실패.");
}

/* ========== 7주차 액션 ========== */

/* 16) 가중치 설정 + 평균 재계산 */
static void action_set_weights(StudentStore* s) {
    int i, n = MAX_SUBJECTS;
    puts("과목별 가중치(기본 1.0). 사용 중인 과목만 의미 있음.");
    for (i = 0;i < n;++i) {
        char msg[64];
#if defined(_MSC_VER)
        _snprintf_s(msg, sizeof(msg), _TRUNCATE, "%d번째 과목 가중치(0.0~5.0): ", i + 1);
#else
        snprintf(msg, sizeof(msg), "%d번째 과목 가중치(0.0~5.0): ", i + 1);
#endif
        g_cfg.subject_weights[i] = prompt_float(msg, 0.0f, 5.0f);
    }
    recompute_all_averages(s, &g_cfg);
    puts("가중치 적용 및 평균/등수 재계산 완료.");
}

/* 17) 합격 컷 설정 */
static void action_set_pass_cut(void) {
    g_cfg.pass_threshold = prompt_float("합격 기준 점수(0~100): ", 0.0f, 100.0f);
    printf("합격 기준이 %.2f 로 설정되었습니다.\n", g_cfg.pass_threshold);
}

/* 18) JSON 내보내기 */
static void action_export_json(const StudentStore* s) {
    char p[260]; prompt_string("JSON 파일명: ", p, sizeof(p));
    if (export_json(p, s, &g_cfg)) puts("JSON 내보내기 완료."); else puts("JSON 내보내기 실패.");
}

/* 19) 페이지로 목록 보기 */
static void action_list_paged(const StudentStore* s) {
    int page = 1, per = prompt_int("페이지 크기(행 수): ", 1, 1000);
    int total = (int)s->size, pages = (total + per - 1) / per;
    if (total == 0) { puts("학생 없음."); return; }
    while (1) {
        int i, start = (page - 1) * per, end = start + per; if (end > total) end = total;
        printf("\n[페이지 %d/%d] (총 %d명)\n", page, pages, total);
        puts("ID\t이름\t평균\t등급\t합격\t등수");
        for (i = start;i < end;++i) {
            const Student* st = &s->data[i];
            printf("%d\t%s\t%.2f\t%s\t%s\t%d\n", st->id, st->name, st->average,
                grade_of_average(st->average), pass_label(st->average, g_cfg.pass_threshold), st->rank);
        }
        if (pages == 1) break;
        puts("n)다음 p)이전 q)종료  숫자)해당 페이지");
        { char buff[16]; if (!fgets(buff, sizeof(buff), stdin)) break;
        if (buff[0] == 'q' || buff[0] == 'Q') break;
        else if (buff[0] == 'n' || buff[0] == 'N') { if (page < pages) page++; }
        else if (buff[0] == 'p' || buff[0] == 'P') { if (page > 1) page--; }
        else { int want; if (sscanf_s(buff, "%d", &want) == 1 && want >= 1 && want <= pages) page = want; }
        }
    }
}

int main(void) {
    StudentStore store; int run = 1; init_store(&store); init_config(&g_cfg);
    while (run) {
        int cmd; print_menu(); cmd = prompt_int("메뉴 선택: ", 0, 19);
        switch (cmd) {
        case 0: run = 0; break;
        case 1: action_add(&store); break;
        case 2: action_list(&store); break;
        case 3: action_save(&store); break;
        case 4: action_load(&store); break;
        case 5: action_update(&store); break;
        case 6: action_delete(&store); break;
        case 7: action_search_exact(&store); break;
        case 8: action_sort_simple(&store); break;
        case 9: action_stats(&store); break;
        case 10: { char key[MAX_NAME_LEN]; Student* matches[128]; size_t i, c;
            prompt_string("이름의 일부: ", key, sizeof(key));
            c = find_students_by_name_substr_ci(&store, key, matches, 128);
            if (!c) puts("없음."); else {
                puts("ID\t이름\t평균\t등급\t합격\t등수");
                for (i = 0;i < c;++i)
                    printf("%d\t%s\t%.2f\t%s\t%s\t%d\n",
                        matches[i]->id, matches[i]->name, matches[i]->average,
                        grade_of_average(matches[i]->average),
                        pass_label(matches[i]->average, g_cfg.pass_threshold),
                        matches[i]->rank);
            } } break;
        case 11: action_sort_advanced(&store); break;
        case 12: action_top_n(&store); break;
        case 13: action_print_card(&store); break;
        case 14: action_export_report(&store); break;
        case 15: action_import_merge(&store); break;
        case 16: action_set_weights(&store); break;   /* 7주차 */
        case 17: action_set_pass_cut(); break;        /* 7주차 */
        case 18: action_export_json(&store); break;   /* 7주차 */
        case 19: action_list_paged(&store); break;    /* 7주차 */
        default: puts("잘못된 선택."); break;
        }
    }
    free_store(&store);
    puts("종료합니다.");
    return EXIT_SUCCESS;
}
