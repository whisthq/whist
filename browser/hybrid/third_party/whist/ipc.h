/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * All rights reserved
 */

#ifndef WHIST_BROWSER_HYBRID_THIRD_PARTY_WHIST_IPC_H_
#define WHIST_BROWSER_HYBRID_THIRD_PARTY_WHIST_IPC_H_

#include <inttypes.h>

namespace base {

enum class CommandType {
  RASTER, // 0
  CREATE_SHARED_MEM, // 1
  CREATE_TRANSFER_CACHE_MEM, // 2
  CREATE_FONT_MEM, // 3
  FLUSH, // 4
  CREATE_SHARED_IMAGE, // 5
  DESTROY_SHARED_IMAGE, // 6
  COMPOSITOR_FRAME, // 7
  GPU_CONTROL, // 8
  ORDERING_BARRIER, // 9
  LOCK_TRANSFER_CACHE, // 10
  INVALID, // 11
};

typedef enum GpuControlType {
  ENSURE_WORK_VISIBLE,
  FLUSH_PENDING_WORK,
  GEN_FENCE_SYNC_RELEASE,
  WAIT_SYNC_TOKEN,
} GpuControlType;

typedef struct GenFenceSyncRelease {
  uint64_t release_count;
} GenFenceSyncRelease;

typedef struct WaitSyncToken {
  uint64_t release_count;
  uint64_t command_buffer_id;
  bool verified_flush;
} WaitSyncToken;

typedef struct GpuControlParams {
  GpuControlType type;
  union {
    GenFenceSyncRelease gen_fence_sync_release;
    WaitSyncToken wait_sync_token;
  };
} GpuControlParams;

typedef struct LockTransferCacheParams {
  uint32_t type;
  uint32_t id;
} LockTransferCacheParams;

}  // namespace base

#endif  // WHIST_BROWSER_HYBRID_THIRD_PARTY_WHIST_IPC_H_
