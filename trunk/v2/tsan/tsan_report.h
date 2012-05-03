//===-- tsan_report.h -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
//===----------------------------------------------------------------------===//
#ifndef TSAN_REPORT_H
#define TSAN_REPORT_H

#include "tsan_defs.h"

namespace __tsan {

enum ReportType {
  ReportTypeRace,
  ReportTypeThreadLeak,
  ReportTypeMutexDestroyLocked,
  ReportTypeSignalUnsafe,
};

struct ReportStack {
  ReportStack *next;
  char *module;
  uptr offset;
  uptr pc;
  char *func;
  char *file;
  int line;
  int col;
};

struct ReportMop {
  int tid;
  uptr addr;
  int size;
  bool write;
  int nmutex;
  int *mutex;
  ReportStack *stack;
};

enum ReportLocationType {
  ReportLocationGlobal,
  ReportLocationHeap,
  ReportLocationStack,
};

struct ReportLocation {
  ReportLocationType type;
  uptr addr;
  uptr size;
  int tid;
  char *name;
  char *file;
  int line;
  ReportStack *stack;
};

struct ReportThread {
  int id;
  bool running;
  char *name;
  ReportStack *stack;
};

struct ReportMutex {
  int id;
  ReportStack *stack;
};

struct ReportDesc {
  ReportType typ;
  ReportStack *stack;
  int nmop;
  ReportMop *mop;
  ReportLocation *loc;
  int nthread;
  ReportThread *thread;
  int nmutex;
  ReportMutex *mutex;
  char alloc[128*1024];
};

// Format and output the report to the console/log. No additional logic.
void PrintReport(const ReportDesc *rep);

class RegionAlloc {
 public:
  RegionAlloc(void *mem, uptr size);
  void *Alloc(uptr size);
  char *Strdup(const char *str);

  template<typename T>
  T *Alloc(uptr cnt) {
    return (T*)this->Alloc(cnt * sizeof(T));
  }

 private:
  char *mem_;
  char *end_;

  RegionAlloc(const RegionAlloc&);  // Not implemented.
  void operator = (const RegionAlloc&);  // Not implemented.
};

}  // namespace __tsan

#endif  // TSAN_REPORT_H
