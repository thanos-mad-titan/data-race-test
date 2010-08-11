/* Copyright (c) 2008-2010, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This file is part of ThreadSanitizer, a dynamic data race detector.
// Author: Konstantin Serebryany.
// Author: Timur Iskhodzhanov.
#ifndef TS_STATS_
#define TS_STATS_

#include "ts_util.h"

// Statistic counters for the entire tool.
struct Stats {
  Stats() {
    memset(this, 0, sizeof(*this));
  }

  void PrintStats() {
    PrintEventStats();
    Printf("   VTS: created small/big: %'ld / %'ld; "
           "deleted: %'ld; cloned: %'ld\n",
           vts_create_small, vts_create_big, vts_delete, vts_clone);
    Printf("   vts_total_size  = %'ld; avg=%'ld\n",
           vts_total_size,
           vts_total_size / (vts_create_small + vts_create_big + 1));
    Printf("   n_seg_hb        = %'ld\n", n_seg_hb);
    Printf("   n_vts_hb        = %'ld\n", n_vts_hb);
    Printf("   n_vts_hb_cached = %'ld\n", n_vts_hb_cached);
    Printf("   memory access:\n"
           "     1: %'ld / %'ld\n"
           "     2: %'ld / %'ld\n"
           "     4: %'ld / %'ld\n"
           "     8: %'ld / %'ld\n"
           "     s: %'ld\n",
           n_fast_access1, n_slow_access1,
           n_fast_access2, n_slow_access2,
           n_fast_access4, n_slow_access4,
           n_fast_access8, n_slow_access8,
           n_very_slow_access);
    PrintStatsForCache();
//    Printf("   Mops:\n"
//           "    total  = %'ld\n"
//           "    unique = %'ld\n",
//           mops_total, mops_uniq);
    Printf("   Publish: set: %'ld; get: %'ld; clear: %'ld\n",
           publish_set, publish_get, publish_clear);

    Printf("   PcTo: all: %'ld rtn: %'ld\n", pc_to_strings, pc_to_rtn_name);

    Printf("   StackTrace: create: %'ld; delete %'ld\n",
           stack_trace_create, stack_trace_delete);

    Printf("   History segments: same: %'ld; reuse: %'ld; new: %'ld\n",
           history_uses_same_segment, history_reuses_segment,
           history_creates_new_segment);
    Printf("   Forget all history: %'ld\n", n_forgets);

    PrintStatsForSeg();
    PrintStatsForSS();
    PrintStatsForLS();
  }

  void PrintStatsForSS() {
    Printf("   SegmentSet: created: %'ld; reused: %'ld;"
           " find: %'ld; recycle: %'ld\n",
           ss_create, ss_reuse, ss_find, ss_recycle);
    Printf("        sizes: 2: %'ld; 3: %'ld; 4: %'ld; other: %'ld\n",
           ss_size_2, ss_size_3, ss_size_4, ss_size_other);

    // SSEq is called at least (ss_find + ss_recycle) times since
    // FindExistingOrAlocateAndCopy calls map_.find()
    // and RecycleOneSegmentSet calls map_.erase(it)
    // Both find() and erase(it) require at least one call to SSHash and SSEq.
    //
    // Apart from SSHash call locations mentioned above,
    // SSHash is called for each AllocateAndCopy (ss_create + ss_reuse) times
    // for insert() AFTER it has already been called
    // by FindExistingOrAlocateAndCopy in case find() returned map_.end().
    // Hence the factor of 2.
    uintptr_t sseq_estimated = ss_find + ss_recycle,
            sshash_estimated = sseq_estimated + 2 * (ss_create + ss_reuse);
    Printf("   SSHash called %12ld times (vs. %12ld = +%d%%)\n"
           "   SSEq   called %12ld times (vs. %12ld = +%d%%)\n",
            sshash_calls, sshash_estimated,
            (sshash_calls - sshash_estimated)/(sshash_estimated/100 + 1),
            sseq_calls,   sseq_estimated,
            (sseq_calls   - sseq_estimated  )/(sseq_estimated/100 + 1));
  }
  void PrintStatsForCache() {
    Printf("   Cache:\n"
           "    fast      = %'ld\n"
           "    new       = %'ld\n"
           "    delete    = %'ld\n"
           "    fetch     = %'ld\n"
           "    storage   = %'ld\n",
           cache_fast_get, cache_new_line,
           cache_delete_empty_line, cache_fetch,
           cache_max_storage_size);
  }

  void PrintStatsForSeg() {
    Printf("   Segment: created: %'ld; reused: %'ld\n",
           seg_create, seg_reuse);
  }

  void PrintStatsForLS() {
    Printf("   LockSet add: 0: %'ld; 1 : %'ld; n : %'ld\n",
           ls_add_to_empty, ls_add_to_singleton, ls_add_to_multi);
    Printf("   LockSet rem: 1: %'ld; n : %'ld\n",
           ls_remove_from_singleton, ls_remove_from_multi);
    Printf("   LockSet cache: add : %'ld; rem : %'ld; fast: %'ld\n",
           ls_add_cache_hit, ls_rem_cache_hit, ls_cache_fast);
  }

  void PrintEventStats() {
    uintptr_t total = 0;
    for (int i = 0; i < LAST_EVENT; i++) {
      if (events[i]) {
        Printf("  %25s: %'ld\n", Event::TypeString((EventType)i),
               events[i]);
      }
      total += events[i];
    }
    Printf("  %25s: %'ld\n", "Total", total);
    for (size_t i = 0; i < TS_ARRAY_SIZE(memory_access_sizes); i++) {
      if (memory_access_sizes[i]) {
        Printf("  mop[%d]: %'ld\n", i, memory_access_sizes[i]);
      }
    }
    for (size_t i = 0; i < TS_ARRAY_SIZE(mops_per_trace); i++) {
      Printf("  mop_per_trace[%d] = %'ld\n", i, mops_per_trace[i]);
    }

    for (size_t i = 0; i < TS_ARRAY_SIZE(lock_sites); i++) {
      if(lock_sites[i] == 0) continue;
      Printf("lock_sites[%ld]=%ld\n", i, lock_sites[i]);
    }
    for (size_t i = 0; i < TS_ARRAY_SIZE(tleb_flush); i++) {
      if(tleb_flush[i] == 0) continue;
      Printf("tleb_flush[%ld]=%ld\n", i, tleb_flush[i]);
    }
    Printf("IgnoreBelowCache miss=%ld\n", ignore_below_cache_miss);
    for (size_t i = 0; i < TS_ARRAY_SIZE(msm_branch_count); i++) {
      if (msm_branch_count[i])
        Printf("msm_branch_count[%02d] = %'ld\n", i, msm_branch_count[i]);
    }
  }


  uintptr_t memory_access_sizes[18];
  uintptr_t events[LAST_EVENT];

  uintptr_t n_vts_hb;
  uintptr_t n_vts_hb_cached;
  uintptr_t n_seg_hb;

  uintptr_t ls_add_to_empty, ls_add_to_singleton, ls_add_to_multi,
            ls_remove_from_singleton, ls_remove_from_multi,
            ls_add_cache_hit, ls_rem_cache_hit,
            ls_cache_fast;

  uintptr_t n_fast_access1, n_fast_access2, n_fast_access4, n_fast_access8,
            n_slow_access1, n_slow_access2, n_slow_access4, n_slow_access8,
            n_very_slow_access, n_access_slow_iter;

  uintptr_t cache_fast_get;
  uintptr_t cache_new_line;
  uintptr_t cache_delete_empty_line;
  uintptr_t cache_fetch;
  uintptr_t cache_max_storage_size;

  uintptr_t mops_total;
  uintptr_t mops_uniq;
  uintptr_t mops_per_trace[16];

  uintptr_t vts_create_big, vts_create_small,
            vts_clone, vts_delete, vts_total_size;

  uintptr_t ss_create, ss_reuse, ss_find, ss_recycle;
  uintptr_t ss_size_2, ss_size_3, ss_size_4, ss_size_other;

  uintptr_t sshash_calls, sseq_calls;

  uintptr_t seg_create, seg_reuse;

  uintptr_t publish_set, publish_get, publish_clear;

  uintptr_t pc_to_strings, pc_to_rtn_name;

  uintptr_t stack_trace_create, stack_trace_delete;

  uintptr_t history_uses_same_segment, history_creates_new_segment,
            history_reuses_segment;

  uintptr_t n_forgets;

  uintptr_t lock_sites[10];

  uintptr_t tleb_flush[10];

  uintptr_t ignore_below_cache_miss;
  uintptr_t msm_branch_count[16];
};


// end. {{{1
#endif  // TS_STATS_
// vim:shiftwidth=2:softtabstop=2:expandtab:tw=80
