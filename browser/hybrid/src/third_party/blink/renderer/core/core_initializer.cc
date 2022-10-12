/*
 * Copyright (c) 2022 Whist Technologies Inc. All rights reserved.
 */

#include "third_party/blink/renderer/core/whist_event_type_names.h"
// Prevent our redefines from modifying headers. We want them to only
// insert our new code into the core_initializer.cc implementation, not
// its #included headers.
#include "third_party/blink/renderer/platform/wtf/text/string_impl.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"

#define ReserveStaticStringsCapacityForSize(X) \
  ReserveStaticStringsCapacityForSize(whist_event_type_names::kNamesCount + X)

#define CSSParserTokenRange \
  whist_event_type_names::Init(); \
  CSSParserTokenRange

#include "src/brave/chromium_src/third_party/blink/renderer/core/core_initializer.cc"
