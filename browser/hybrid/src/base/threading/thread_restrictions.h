/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * All rights reserved
 */

#ifndef WHIST_BROWSER_HYBRID_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_
#define WHIST_BROWSER_HYBRID_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_

namespace extensions {
namespace api {
class WhistUpdatePoliciesFunction;
}  // namespace api
}  // namespace extensions

#define BRAVE_SCOPED_ALLOW_BLOCKING_H \
  friend class extensions::api::WhistUpdatePoliciesFunction;
#include "src/brave/chromium_src/base/threading/thread_restrictions.h"
#undef BRAVE_SCOPED_ALLOW_BLOCKING_H

#endif  // WHIST_BROWSER_HYBRID_SRC_BASE_THREADING_THREAD_RESTRICTIONS_H_
