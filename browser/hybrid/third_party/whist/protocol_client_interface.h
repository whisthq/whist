// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

// This file is inspired from init_webrtc.h, and we export WhistClient-related
// functions following the same permissions/scope as WebRTC, given the
// similarities.

#ifndef THIRD_PARTY_WHIST_PROTOCOL_CLIENT_INTERFACE_H_
#define THIRD_PARTY_WHIST_PROTOCOL_CLIENT_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>

// WebRTC export header -- used to properly set symbol visibility.
#include "third_party/webrtc/rtc_base/system/rtc_export.h"

namespace WhistClient {
#include "src/whist/protocol/client/frontend/virtual/interface.h"
}

// Initialize WhistClient. Call this explicitly to initialize the WhistClient
// library (before initializing the sandbox in Chrome) and hook up the
// Chrome+WhistClient integration such as passing of streaming data between
// Chrome and WhistClient.
RTC_EXPORT void InitializeWhistClient();

RTC_EXPORT extern const WhistClient::VirtualInterface* whist_virtual_interface;

// Using separate U and V allows for casting flexibility, since args
// don't need to strictly match the function prototype.
template <typename T, typename... U, typename... V>
T MaybeCallFunction(T (*call)(U...), V... args) {
  return call ? call(args...) : T();
}

#define WHIST_VIRTUAL_INTERFACE_CALL(name, ...)                       \
  MaybeCallFunction(                                                  \
      whist_virtual_interface ? whist_virtual_interface->name : NULL, \
      ##__VA_ARGS__)

#endif  // THIRD_PARTY_WHIST_PROTOCOL_CLIENT_INTERFACE_H_
