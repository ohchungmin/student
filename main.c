/* main.c : 8주차 통합 메뉴
   - 1~19 : 기존 기능
   - 20   : 자동 백업/되돌리기
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "model.h"
#include "io.h"

// -------- 전역 스토어 --------
static StudentStore g;             // 전체 데이터
static int g_auto_backup = 1;      // 자동 백업 ON

// -------- 입력 유틸 --------
static void flush_stdin(void) { int c; while ((c = getchar()) != '\n' && c != EOF) {} }
static int input_int(const char* label) {
    int x;
    while (1) {
        printf("%s: ", label);
        if (scanf("%d", &x) == 1) { flush_stdin(); return x; }
        flush_stdin(); puts("정수를 입력하세요.");
    }
}
static void input_str(const char* label, char* buf, int cap) {
    printf("%s: ", label);
    if (fgets(buf, cap, stdin) == NULL) { buf[0] = 0; return; }
    size_t n = strlen(buf); if (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[n - 1] = 0;
}

// -------- 공통 동작 --------
static void after_mutation_autobackup(void) {
    if (!g_auto_backup) return;
    save_to_csv_at(BACKUP_PATH_DEFAULT, &g);
}

// -------- 메뉴 렌더 --------
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
    puts("20) 자동 백업/되돌리기  ← 8주차 추가");
    puts("0) 종료");
}

// -------- 기능 구현 --------
static void action_add() {
    Student x;
    x.id = input_int("학번");
    input_str("이름", x.name, NAME_LEN);
    x.kor = input_int("국어(0~100)");
    x.eng = input_int("영어(0~100)");
    x.math = input_int("수학(0~100)");
    if (!store_add(&g, x)) puts("추가 실패(학번 중복 또는 가득 참).");
    else { puts("추가 완료."); after_mutation_autobackup(); }
}
static void action_list() {
    store_list_active(&g);
}
static void action_save() {
    if (save_to_csv(&g, DEFAULT_SAVE_PATH) == 0) puts("저장 완료.");
    else puts("저장 실패.");
}
static void action_load() {
    if (load_from_csv(&g, DEFAULT_SAVE_PATH) >= 0) {
        store_recalc_all(&g);
        puts("불러오기 완료.");
        after_mutation_autobackup();
    }
    else puts("불러오기 실패.");
}
static void action_edit() {
    int id = input_int("수정할 학번");
    puts("1) 점수 수정  2) 이름 수정");
    int t = input_int("선택");
    if (t == 1) {
        int k = input_int("국어"), e = input_int("영어"), m = input_int("수학");
        if (store_edit_scores(&g, id, k, e, m)) { puts("수정 완료."); after_mutation_autobackup(); }
        else puts("수정 실패.");
    }
    else {
        char nm[NAME_LEN]; input_str("새 이름", nm, NAME_LEN);
        if (store_edit_name(&g, id, nm)) { puts("수정 완료."); after_mutation_autobackup(); }
        else puts("수정 실패.");
    }
}
static void action_delete() {
    int id = input_int("삭제할 학번");
    if (store_soft_delete(&g, id)) { puts("삭제 표시 완료."); after_mutation_autobackup(); }
    else puts("삭제 실패.");
}
static void action_search_basic() {
    puts("1) 학번으로 검색  2) 이름(정확 일치)");
    int t = input_int("선택");
    if (t == 1) {
        int id = input_int("학번");
        Student x; if (store_search_id(&g, id, &x)) { print_student_header(); print_student_row(&x); }
        else puts("결과 없음.");
    }
    else {
        char nm[NAME_LEN]; input_str("이름", nm, NAME_LEN);
        Student tmp[128]; int n = store_search_name_exact(&g, nm, tmp, 128);
        if (!n) puts("결과 없음.");
        else { print_student_header(); for (int i = 0;i < n;i++) print_student_row(&tmp[i]); }
    }
}
static void action_sort_simple() {
    puts("정렬 기준: 1) 평균  2) 이름  3) 총점");
    int k = input_int("선택");
    puts("순서: 1) 오름차순  2) 내림차순");
    int a = input_int("선택");
    SortKey key = (k == 2) ? SORT_BY_NAME : (k == 3 ? SORT_BY_TOTAL : SORT_BY_AVG);
    int asc = (a == 2) ? 0 : 1;
    store_sort(&g, key, asc);
    puts("정렬 완료. 현재 순서로 목록을 보여줍니다.");
    store_list_active(&g);
}
static void action_stats() {
    store_subject_stats(&g, "과목별 통계");
}
static void action_search_advanced() {
    char key[NAME_LEN]; input_str("이름 일부(대소문자 무시)", key, NAME_LEN);
    Student tmp[256]; int n = store_search_name_partial_ci(&g, key, tmp, 256);
    if (!n) puts("결과 없음.");
    else { print_student_header(); for (int i = 0;i < n;i++) print_student_row(&tmp[i]); }
}
static void action_sort_multi() {
    puts("1차 키: 1) 평균 2) 이름 3) 총점");
    int k1 = input_int("선택"); puts("1차 순서: 1) 오름 2) 내림"); int a1 = input_int("선택");
    puts("2차 키: 1) 평균 2) 이름 3) 총점");
    int k2 = input_int("선택"); puts("2차 순서: 1) 오름 2) 내림"); int a2 = input_int("선택");
    store_sort_multi(&g, (k1 == 2 ? SORT_BY_NAME : (k1 == 3 ? SORT_BY_TOTAL : SORT_BY_AVG)), a1 == 1,
        (k2 == 2 ? SORT_BY_NAME : (k2 == 3 ? SORT_BY_TOTAL : SORT_BY_AVG)), a2 == 1);
    puts("다중 정렬 완료.");
    store_list_active(&g);
}
static void action_top_n() {
    int n = input_int("상위 N");
    Student tmp[256]; int m = store_top_n(&g, n, tmp, 256);
    if (!m) puts("데이터 없음.");
    else { print_student_header(); for (int i = 0;i < m;i++) print_student_row(&tmp[i]); }
}
static void action_print_one() {
    int id = input_int("학번");
    Student x; if (store_search_id(&g, id, &x)) print_single_report_card(&x, &g.pass_rule);
    else puts("없음.");
}
static void action_export_txt() {
    if (export_all_reports_txt(&g, DEFAULT_TXT_DIR) == 0) puts("TXT 내보내기 완료.");
    else puts("실패.");
}
static void action_merge_csv() {
    char path[256]; input_str("불러올 CSV 경로", path, 256);
    puts("모드: 0) 덮어쓰기  1) 병합");
    int m = input_int("선택");
    int r = merge_from_csv(&g, path, m);
    if (r < 0) puts("머지 실패.");
    else { printf("머지 처리: %d건.\n", r); after_mutation_autobackup(); }
}
static void action_weights() {
    double wk, we, wm;
    printf("국어 가중치: "); scanf("%lf", &wk); flush_stdin();
    printf("영어 가중치: "); scanf("%lf", &we); flush_stdin();
    printf("수학 가중치: "); scanf("%lf", &wm); flush_stdin();
    store_set_weights(&g, wk, we, wm);
    puts("가중치 반영 및 평균/학점 재계산 완료.");
    after_mutation_autobackup();
}
static void action_passcut() {
    double cut; printf("합격 컷(평균): "); scanf("%lf", &cut); flush_stdin();
    store_set_pass_cut(&g, cut);
    puts("합격 기준 저장.");
}
static void action_export_json() {
    if (export_to_json(&g, DEFAULT_JSON_PATH) >= 0) puts("JSON 내보내기 완료.");
    else puts("실패.");
}
static void action_paged_list() {
    int page = input_int("페이지 번호(1부터)");
    int per = input_int("페이지 크기");
    int total = store_count_active(&g);
    int start = (page - 1) * per, shown = 0, idx = 0;
    print_student_header();
    for (int i = 0;i < g.count;i++) {
        Student* p = &g.arr[i]; if (p->deleted) continue;
        if (idx >= start && shown < per) { print_student_row(p); shown++; }
        idx++;
    }
    printf("[페이지 %d] 총 %d명 중 %d명 표시\n", page, total, shown);
}
static void action_backup_menu() {
    printf("\n[자동 백업/되돌리기] 현재: %s\n", g_auto_backup ? "ON" : "OFF");
    puts("1) 자동 백업 ON");
    puts("2) 자동 백업 OFF");
    puts("3) 지금 즉시 백업");
    puts("4) 백업에서 되돌리기");
    puts("0) 뒤로");
    int s = input_int("선택");
    if (s == 1) { g_auto_backup = 1; puts("ON"); }
    else if (s == 2) { g_auto_backup = 0; puts("OFF"); }
    else if (s == 3) {
        if (save_to_csv_at(BACKUP_PATH_DEFAULT, &g) == 0) puts("백업 완료.");
        else puts("백업 실패.");
    }
    else if (s == 4) {
        StudentStore tmp; store_init(&tmp);
        if (load_from_csv_at(BACKUP_PATH_DEFAULT, &tmp) >= 0) {
            store_recalc_all(&tmp); g = tmp;
            puts("되돌리기 완료.");
            save_to_csv(&g, DEFAULT_SAVE_PATH);
        }
        else puts("백업 파일 없음/손상.");
    }
}

// -------- main --------
int main(void) {
    printf("[BUILD %s %s]\n", __DATE__, __TIME__);
    store_init(&g);

    // 시작 시 자동 로드
    if (load_from_csv(&g, DEFAULT_SAVE_PATH) >= 0) store_recalc_all(&g);

    while (1) {
        print_menu();
        int sel = input_int("메뉴 선택");
        switch (sel) {
        case 0: action_save(); puts("종료."); return 0;
        case 1: action_add(); break;
        case 2: action_list(); break;
        case 3: action_save(); break;
        case 4: action_load(); break;
        case 5: action_edit(); break;
        case 6: action_delete(); break;
        case 7: action_search_basic(); break;
        case 8: action_sort_simple(); break;
        case 9: action_stats(); break;
        case 10: action_search_advanced(); break;
        case 11: action_sort_multi(); break;
        case 12: action_top_n(); break;
        case 13: action_print_one(); break;
        case 14: action_export_txt(); break;
        case 15: action_merge_csv(); break;
        case 16: action_weights(); break;
        case 17: action_passcut(); break;
        case 18: action_export_json(); break;
        case 19: action_paged_list(); break;
        case 20: action_backup_menu(); break;
        default: puts("잘못된 선택."); break;
        }
    }
}
