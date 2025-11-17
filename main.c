/* main.c : 8주차 통합 메뉴 + 9주차 추가 기능
   - 1~19 : 기존 기능
   - 20   : 자동 백업/되돌리기 (8주차)
   - 21   : 성적 분포(평균 기준 구간별 인원 수) 보기 (9주차에서 새로 추가한 기능)
   - 9주차에서는 "성적 분포"라는 새 메뉴를 추가했고,
     처리 과정(계산/누적/출력) 부분에도 전부 주석을 달아서
     C를 많이 모르는 사람도 흐름을 따라갈 수 있게 했다.
*/
#define _CRT_SECURE_NO_WARNINGS      /* Visual Studio에서 scanf, fopen 같은 함수 쓸 때 나오는 보안 경고 끄기 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "model.h"                   /* Student, StudentStore, 정렬/검색 관련 함수 선언 */
#include "io.h"                      /* CSV/TXT/JSON, 백업 관련 입출력 함수 선언 */

// -------- 전역 스토어 --------
/* 프로그램 전체에서 하나의 학생 목록을 공유해서 쓰기 위해 전역 변수로 둔다 */
static StudentStore g;               // 전체 학생/설정 데이터가 들어 있는 구조체
static int g_auto_backup = 1;        // 자동 백업 ON/OFF (1 = ON, 0 = OFF)

// -------- 입력 유틸 --------
/* 표준 입력 버퍼를 비워주는 함수
   - scanf 후에 남아 있는 '\n' 같은 걸 날려서 다음 입력이 꼬이지 않게 한다. */
static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

/* 정수 입력 전용 함수
   - label: "학번", "메뉴 선택" 같은 안내 문구
   - 정수가 들어올 때까지 계속 반복해서 물어본다. */
static int input_int(const char* label) {
    int x;
    while (1) {
        printf("%s: ", label);          // 프롬프트 출력
        if (scanf("%d", &x) == 1) {     // 정수 하나 입력 성공 시
            flush_stdin();              // 뒤에 남아 있는 개행 문자 제거
            return x;                   // 입력된 값 반환
        }
        // 숫자가 아닌 값이 들어온 경우 여기로 옴
        flush_stdin();                  // 잘못 들어온 입력 전체 버림
        puts("정수를 입력하세요.");
    }
}

/* 문자열 한 줄 입력 함수
   - 공백 포함 이름, 경로 등을 입력받을 때 사용
   - 마지막에 붙는 '\n' 문자를 제거해 준다. */
static void input_str(const char* label, char* buf, int cap) {
    printf("%s: ", label);
    if (fgets(buf, cap, stdin) == NULL) {  // 입력 실패 시
        buf[0] = 0;                        // 빈 문자열로 처리
        return;
    }
    // 버퍼 끝에 붙은 '\n' 또는 '\r' 제거
    size_t n = strlen(buf);
    if (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[n - 1] = 0;
}

// -------- 공통 동작 --------
/* 학생 추가/수정/삭제처럼 "데이터가 바뀌는" 작업이 끝난 뒤에 호출
   - g_auto_backup이 1(ON)일 때만 BACKUP_PATH_DEFAULT로 자동 백업을 수행한다. */
static void after_mutation_autobackup(void) {
    if (!g_auto_backup) return;                     // 자동 백업 OFF면 아무 것도 안 함
    save_to_csv_at(BACKUP_PATH_DEFAULT, &g);        // 전역 스토어 g를 CSV로 저장
}

// -------- 메뉴 렌더 --------
/* 현재 프로그램에서 제공하는 기능 목록을 화면에 보여주는 함수
   - 사용자는 여기서 번호를 보고 선택만 하면 된다. */
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
    puts("21) 성적 분포(구간별 인원 수) 보기  ← 9주차 추가 기능");
    puts("0) 종료");
}

// -------- 기능 구현 --------

/* (1) 학생 추가
   - 입력: 학번, 이름, 국어/영어/수학 점수
   - 처리: store_add로 전역 스토어 g에 학생 추가
   - 후처리: 성공 시 자동 백업 */
static void action_add() {
    Student x;                          // 새로 넣을 학생 구조체

    x.id = input_int("학번");
    input_str("이름", x.name, NAME_LEN);
    x.kor  = input_int("국어(0~100)");
    x.eng  = input_int("영어(0~100)");
    x.math = input_int("수학(0~100)");

    // store_add 내부에서 학번 중복, 공간 부족 등을 체크해 준다.
    if (!store_add(&g, x))
        puts("추가 실패(학번 중복 또는 가득 참).");
    else {
        puts("추가 완료.");
        after_mutation_autobackup();    // 데이터가 변했으니 자동 백업
    }
}

/* (2) 학생 목록 보기
   - 삭제되지 않은 학생들만 표 형태로 출력한다. */
static void action_list() {
    store_list_active(&g);
}

/* (3) 저장 (CSV)
   - DEFAULT_SAVE_PATH 경로에 현재 g 내용을 CSV 파일로 저장 */
static void action_save() {
    if (save_to_csv(&g, DEFAULT_SAVE_PATH) == 0)
        puts("저장 완료.");
    else
        puts("저장 실패.");
}

/* (4) 불러오기 (CSV)
   - DEFAULT_SAVE_PATH에서 CSV를 읽어와 g를 채우고,
   - store_recalc_all로 평균, 학점 등 파생 정보들을 다시 계산한다.
   - 불러오기 또한 "데이터 변경"이므로 자동 백업 처리 대상. */
static void action_load() {
    if (load_from_csv(&g, DEFAULT_SAVE_PATH) >= 0) {
        store_recalc_all(&g);
        puts("불러오기 완료.");
        after_mutation_autobackup();
    }
    else puts("불러오기 실패.");
}

/* (5) 학생 점수/이름 수정
   - 먼저 학번으로 학생을 찾고,
   - 점수만 수정할지, 이름만 수정할지 사용자가 선택한다. */
static void action_edit() {
    int id = input_int("수정할 학번");
    puts("1) 점수 수정  2) 이름 수정");
    int t = input_int("선택");

    if (t == 1) {
        // 점수 수정 모드
        int k = input_int("국어"), e = input_int("영어"), m = input_int("수학");
        if (store_edit_scores(&g, id, k, e, m)) {
            puts("수정 완료.");
            after_mutation_autobackup();
        }
        else puts("수정 실패.");
    }
    else {
        // 이름 수정 모드
        char nm[NAME_LEN];
        input_str("새 이름", nm, NAME_LEN);
        if (store_edit_name(&g, id, nm)) {
            puts("수정 완료.");
            after_mutation_autobackup();
        }
        else puts("수정 실패.");
    }
}

/* (6) 학생 삭제
   - 실제 데이터를 지우는 것이 아니라 store_soft_delete로
     "삭제 플래그"만 세운다(소프트 삭제).
   - 목록, 검색 등에서는 삭제된 학생이 보이지 않는다. */
static void action_delete() {
    int id = input_int("삭제할 학번");
    if (store_soft_delete(&g, id)) {
        puts("삭제 표시 완료.");
        after_mutation_autobackup();
    }
    else puts("삭제 실패.");
}

/* (7) 학생 검색(정확 일치)
   - 1) 학번으로 검색
   - 2) 이름 정확 일치로 검색
   - 검색 결과는 표 형태로 출력한다. */
static void action_search_basic() {
    puts("1) 학번으로 검색  2) 이름(정확 일치)");
    int t = input_int("선택");

    if (t == 1) {
        int id = input_int("학번");
        Student x;
        if (store_search_id(&g, id, &x)) {
            print_student_header();
            print_student_row(&x);
        }
        else puts("결과 없음.");
    }
    else {
        char nm[NAME_LEN];
        input_str("이름", nm, NAME_LEN);
        Student tmp[128];
        int n = store_search_name_exact(&g, nm, tmp, 128);
        if (!n) puts("결과 없음.");
        else {
            print_student_header();
            for (int i = 0; i < n; i++)
                print_student_row(&tmp[i]);
        }
    }
}

/* (8) 정렬(평균/이름/총점)
   - 사용자가 선택한 기준과(평균/이름/총점) 오름/내림 옵션을
     SortKey, asc 플래그로 변환해서 store_sort에 전달한다. */
static void action_sort_simple() {
    puts("정렬 기준: 1) 평균  2) 이름  3) 총점");
    int k = input_int("선택");
    puts("순서: 1) 오름차순  2) 내림차순");
    int a = input_int("선택");

    // 메뉴 번호 → SortKey로 매핑
    SortKey key = (k == 2) ? SORT_BY_NAME : (k == 3 ? SORT_BY_TOTAL : SORT_BY_AVG);
    // 오름차순(1) / 내림차순(0) 결정
    int asc = (a == 2) ? 0 : 1;

    // 실제 정렬 수행
    store_sort(&g, key, asc);
    puts("정렬 완료. 현재 순서로 목록을 보여줍니다.");
    store_list_active(&g);
}

/* (9) 과목별 통계
   - 과목별 평균, 최대/최소 등 통계를 store_subject_stats로 출력한다. */
static void action_stats() {
    store_subject_stats(&g, "과목별 통계");
}

/* (10) 고급 검색(부분/대소문자 무시)
   - 이름 일부만 입력해도 되고, 대소문자를 구분하지 않고 검색한다.
   - 결과는 최대 256명까지 tmp 배열에 담아서 출력. */
static void action_search_advanced() {
    char key[NAME_LEN];
    input_str("이름 일부(대소문자 무시)", key, NAME_LEN);
    Student tmp[256];
    int n = store_search_name_partial_ci(&g, key, tmp, 256);
    if (!n) puts("결과 없음.");
    else {
        print_student_header();
        for (int i = 0; i < n; i++)
            print_student_row(&tmp[i]);
    }
}

/* (11) 다중 기준 정렬
   - 1차/2차 키를 각각 선택(평균, 이름, 총점)하고
   - 각 키의 오름/내림 여부를 선택한 뒤,
   - store_sort_multi로 복잡한 정렬 기준을 한 번에 적용한다. */
static void action_sort_multi() {
    puts("1차 키: 1) 평균 2) 이름 3) 총점");
    int k1 = input_int("선택");
    puts("1차 순서: 1) 오름 2) 내림");
    int a1 = input_int("선택");

    puts("2차 키: 1) 평균 2) 이름 3) 총점");
    int k2 = input_int("선택");
    puts("2차 순서: 1) 오름 2) 내림");
    int a2 = input_int("선택");

    // 메뉴 번호를 실제 정렬 키/순서로 변환해서 전달
    store_sort_multi(
        &g,
        (k1 == 2 ? SORT_BY_NAME : (k1 == 3 ? SORT_BY_TOTAL : SORT_BY_AVG)), a1 == 1,
        (k2 == 2 ? SORT_BY_NAME : (k2 == 3 ? SORT_BY_TOTAL : SORT_BY_AVG)), a2 == 1
    );
    puts("다중 정렬 완료.");
    store_list_active(&g);
}

/* (12) 상위 N명 보기
   - 예: 상위 3명, 상위 5명 등
   - store_top_n에서 상위 N명을 tmp 배열에 담아주면 그것만 출력한다. */
static void action_top_n() {
    int n = input_int("상위 N");
    Student tmp[256];
    int m = store_top_n(&g, n, tmp, 256);
    if (!m) puts("데이터 없음.");
    else {
        print_student_header();
        for (int i = 0; i < m; i++)
            print_student_row(&tmp[i]);
    }
}

/* (13) 학생 성적표(1명) 출력
   - 학번으로 학생을 찾고, print_single_report_card로
     해당 학생의 자세한 성적표를 출력한다. */
static void action_print_one() {
    int id = input_int("학번");
    Student x;
    if (store_search_id(&g, id, &x))
        print_single_report_card(&x, &g.pass_rule);
    else
        puts("없음.");
}

/* (14) 전체 리포트 TXT로 내보내기
   - DEFAULT_TXT_DIR 경로에 전체 학생 성적표를 텍스트 파일 형식으로 내보낸다. */
static void action_export_txt() {
    if (export_all_reports_txt(&g, DEFAULT_TXT_DIR) == 0)
        puts("TXT 내보내기 완료.");
    else
        puts("실패.");
}

/* (15) CSV 머지 불러오기
   - 다른 CSV 파일을 불러와서
     0) 덮어쓰기 모드 또는 1) 병합 모드로 처리한다.
   - merge_from_csv의 결과값: 실제로 처리된 레코드 수 */
static void action_merge_csv() {
    char path[256];
    input_str("불러올 CSV 경로", path, 256);
    puts("모드: 0) 덮어쓰기  1) 병합");
    int m = input_int("선택");

    int r = merge_from_csv(&g, path, m);
    if (r < 0) puts("머지 실패.");
    else {
        printf("머지 처리: %d건.\n", r);
        after_mutation_autobackup();    // 머지 후에도 데이터가 변하므로 자동 백업
    }
}

/* (16) 과목별 가중치 설정 + 평균 재계산
   - 국어/영어/수학의 비중을 사용자가 직접 입력
   - store_set_weights가 내부 가중치를 바꾸고,
     모든 학생의 평균/학점을 다시 계산한다. */
static void action_weights() {
    double wk, we, wm;
    printf("국어 가중치: "); scanf("%lf", &wk); flush_stdin();
    printf("영어 가중치: "); scanf("%lf", &we); flush_stdin();
    printf("수학 가중치: "); scanf("%lf", &wm); flush_stdin();

    store_set_weights(&g, wk, we, wm);
    puts("가중치 반영 및 평균/학점 재계산 완료.");
    after_mutation_autobackup();
}

/* (17) 합격 기준 설정
   - 평균이 cut 이상이면 합격으로 보는 기준을 설정한다. */
static void action_passcut() {
    double cut;
    printf("합격 컷(평균): ");
    scanf("%lf", &cut);
    flush_stdin();
    store_set_pass_cut(&g, cut);
    puts("합격 기준 저장.");
}

/* (18) JSON 내보내기
   - DEFAULT_JSON_PATH 경로에 JSON 형식으로 학생 데이터를 저장 */
static void action_export_json() {
    if (export_to_json(&g, DEFAULT_JSON_PATH) >= 0)
        puts("JSON 내보내기 완료.");
    else
        puts("실패.");
}

/* (19) 페이지로 목록 보기
   - 페이지 번호(page)와 페이지당 학생 수(per)를 입력하면
     해당 구간의 학생들만 잘라서 보여준다. */
static void action_paged_list() {
    int page = input_int("페이지 번호(1부터)");
    int per  = input_int("페이지 크기");

    int total = store_count_active(&g);      // 삭제되지 않은 전체 학생 수
    int start = (page - 1) * per;           // 이 페이지에서 시작할 index(0 기준)
    int shown = 0;                          // 실제 화면에 보여준 학생 수
    int idx   = 0;                          // active 학생들을 순서대로 세는 용도

    print_student_header();
    for (int i = 0; i < g.count; i++) {
        Student* p = &g.arr[i];
        if (p->deleted) continue;           // 삭제된 학생은 건너뛴다.

        // idx가 start 이상이면 "이 페이지에 들어갈 학생"이라는 뜻
        if (idx >= start && shown < per) {
            print_student_row(p);           // 한 줄 출력
            shown++;                        // 이 페이지에서 출력한 인원 수 +1
        }
        idx++;                              // active 학생 index 증가
    }
    printf("[페이지 %d] 총 %d명 중 %d명 표시\n", page, total, shown);
}

/* (21) 성적 분포(평균 기준 구간별 인원 수) – 9주차 추가 기능
   - 이 함수가 9주차에 새로 추가된 핵심 기능이다.
   - 처리 방식(= '처리된 부분'):
     1) bins[5] 배열을 0으로 초기화해 각 구간 인원 수를 저장.
     2) g.arr 전체를 돌면서, 삭제되지 않은 학생의 평균 점수를 계산.
     3) 평균 점수에 따라 해당 구간(0~59, 60~69, 70~79, 80~89, 90~100)에 +1.
     4) 마지막에 각 구간에 몇 명이 있는지 한 번에 출력.
*/
static void action_score_distribution() {
    // bins[0] : 0~59점 인원 수
    // bins[1] : 60~69점 인원 수
    // bins[2] : 70~79점 인원 수
    // bins[3] : 80~89점 인원 수
    // bins[4] : 90~100점 인원 수
    int bins[5] = {0,};           // 모든 구간 인원 수 0으로 초기화
    int total = 0;                // 전체(삭제 안 된) 학생 인원 수

    // g.arr에 들어 있는 전체 학생을 순회
    for (int i = 0; i < g.count; i++) {
        Student* p = &g.arr[i];

        if (p->deleted)          // 삭제 표시된 학생이면
            continue;            // 이 학생은 분포 계산에서 제외

        // 이 학생의 간단 평균(국어/영어/수학) 계산
        double avg = (p->kor + p->eng + p->math) / 3.0;
        total++;                 // 유효한 학생 수 +1

        // 평균 점수가 어느 구간에 속하는지에 따라 해당 bins 요소를 +1
        if (avg < 60)           // 0 ~ 59.xx
            bins[0]++;
        else if (avg < 70)      // 60 ~ 69.xx
            bins[1]++;
        else if (avg < 80)      // 70 ~ 79.xx
            bins[2]++;
        else if (avg < 90)      // 80 ~ 89.xx
            bins[3]++;
        else                    // 90 ~ 100
            bins[4]++;
    }

    // 계산이 끝났으니 결과 출력
    printf("\n[성적 분포(평균 기준)]\n");
    printf("총 인원: %d명\n", total);
    printf("0~59점   : %d명\n", bins[0]);
    printf("60~69점  : %d명\n", bins[1]);
    printf("70~79점  : %d명\n", bins[2]);
    printf("80~89점  : %d명\n", bins[3]);
    printf("90~100점 : %d명\n", bins[4]);
}

/* (20) 자동 백업/되돌리기 서브 메뉴
   - g_auto_backup ON/OFF 설정
   - 지금 즉시 백업
   - 백업 파일에서 되돌리기
   를 모아놓은 작은 메뉴이다. */
static void action_backup_menu() {
    printf("\n[자동 백업/되돌리기] 현재: %s\n", g_auto_backup ? "ON" : "OFF");
    puts("1) 자동 백업 ON");
    puts("2) 자동 백업 OFF");
    puts("3) 지금 즉시 백업");
    puts("4) 백업에서 되돌리기");
    puts("0) 뒤로");

    int s = input_int("선택");

    if (s == 1) {
        g_auto_backup = 1;
        puts("ON");
    }
    else if (s == 2) {
        g_auto_backup = 0;
        puts("OFF");
    }
    else if (s == 3) {
        // 현재 g 내용을 BACKUP_PATH_DEFAULT에 강제 저장
        if (save_to_csv_at(BACKUP_PATH_DEFAULT, &g) == 0)
            puts("백업 완료.");
        else
            puts("백업 실패.");
    }
    else if (s == 4) {
        // 되돌리기: 임시 스토어 tmp에 먼저 로드 → 성공하면 g에 덮어쓰기
        StudentStore tmp;
        store_init(&tmp);    // tmp를 초기화 (빈 스토어)

        if (load_from_csv_at(BACKUP_PATH_DEFAULT, &tmp) >= 0) {
            store_recalc_all(&tmp);  // 읽어 온 데이터의 평균/학점 재계산
            g = tmp;                 // 전역 스토어 g 전체를 tmp 내용으로 교체
            puts("되돌리기 완료.");
            // 되돌린 내용을 기본 저장 파일에도 저장해 두기
            save_to_csv(&g, DEFAULT_SAVE_PATH);
        }
        else {
            puts("백업 파일 없음/손상.");
        }
    }
    // 0번이면 아무 일도 안 하고 함수 종료 → 메인 메뉴로 복귀
}

// -------- main --------
/* 프로그램 시작 지점
   - 전역 스토어 초기화
   - 기존 CSV 자동 로드
   - 무한 루프로 메뉴 처리
*/
int main(void) {
    // 빌드된 날짜/시간 찍어주기(디버깅용)
    printf("[BUILD %s %s]\n", __DATE__, __TIME__);

    // 전역 스토어 g 초기화(배열 비우기, 기본 설정값 등)
    store_init(&g);

    // 시작할 때 기존 CSV 파일이 있으면 자동으로 한 번 불러오기
    if (load_from_csv(&g, DEFAULT_SAVE_PATH) >= 0)
        store_recalc_all(&g);

    // 메인 메뉴 루프: 0(종료)를 선택할 때까지 계속 반복
    while (1) {
        print_menu();                       // 메뉴 출력
        int sel = input_int("메뉴 선택");   // 번호 입력

        // 선택된 메뉴 번호에 따라 대응되는 기능(action_*) 호출
        switch (sel) {
        case 0:
            // 종료 전에 한 번 더 저장하고 프로그램 종료
            action_save();
            puts("종료.");
            return 0;

        case 1:  action_add();              break;
        case 2:  action_list();             break;
        case 3:  action_save();             break;
        case 4:  action_load();             break;
        case 5:  action_edit();             break;
        case 6:  action_delete();           break;
        case 7:  action_search_basic();     break;
        case 8:  action_sort_simple();      break;
        case 9:  action_stats();            break;
        case 10: action_search_advanced();  break;
        case 11: action_sort_multi();       break;
        case 12: action_top_n();            break;
        case 13: action_print_one();        break;
        case 14: action_export_txt();       break;
        case 15: action_merge_csv();        break;
        case 16: action_weights();          break;
        case 17: action_passcut();          break;
        case 18: action_export_json();      break;
        case 19: action_paged_list();       break;
        case 20: action_backup_menu();      break;
        case 21: action_score_distribution(); break;  // 9주차 새 기능: 성적 분포 계산/출력
        default:
            puts("잘못된 선택.");
            break;
        }
    }
}
