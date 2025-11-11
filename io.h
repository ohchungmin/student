#pragma once
// io.h : 파일 입출력(CSV/JSON/TXT) 및 경로 상수

#ifndef IO_H
#define IO_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

	// 기본 저장 경로(프로그램 폴더)
#define DEFAULT_SAVE_PATH   "students.csv"
#define DEFAULT_JSON_PATH   "students.json"
#define DEFAULT_TXT_DIR     "reports.txt"   // 간단히 하나의 TXT에 모아서 저장
#define BACKUP_PATH_DEFAULT "students_backup.csv"

// CSV 저장/불러오기
	int save_to_csv(const StudentStore* s, const char* path);
	int load_from_csv(StudentStore* s, const char* path);

	// 편의 alias (main에서 호출)
	static inline int save_to_csv_at(const char* path, const StudentStore* s) { return save_to_csv(s, path); }
	static inline int load_from_csv_at(const char* path, StudentStore* s) { return load_from_csv(s, path); }

	// JSON 내보내기
	int export_to_json(const StudentStore* s, const char* path);

	// TXT 전체 리포트
	int export_all_reports_txt(const StudentStore* s, const char* path);

	// CSV 병합(덮어쓰기 or 병합)
	// mode: 0=덮어쓰기(기존 전부 교체), 1=병합(같은 학번은 업데이트, 없으면 추가)
	int merge_from_csv(StudentStore* s, const char* path, int mode);

#ifdef __cplusplus
}
#endif

#endif // IO_H
