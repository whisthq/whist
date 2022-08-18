// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_MEDIASTREAM_WHIST_SOURCE_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_MEDIASTREAM_WHIST_SOURCE_H_

namespace blink {

/**
 * WhistSource is a dummy WebMediaPlayerSource which represents a
 * connection between Chromium and the Whist protocol. It is primarily
 * used to create a WhistPlayer on initialization of an HTMLWhistElement.
 *
 * Unlike other WebMediaPlayerSource types, WhistSource does not provide
 * video frames to WhistPlayer; instead, WhistPlayer directly interfaces
 * with the protocol. Likewise, HTMLWhistElement communicates directly
 * with the protocol for events and CSS updates.
 *
 * This class takes on a bit more importance when it comes to shared
 * functionality between HTMLWhistElement and WhistPlayer. For example,
 * based on focus/blur events in HTMLWhistElement, we may want to stop
 * or start frame submission to WhistPlayer. Our mechanism for doing this
 * in the protocol when multiple cloud tabs exist is to index by a
 * "window_id". But we need some way to associate the same `window_id`
 * with both HTMLWhistElement and WhistPlayer. Hence, we add this
 * responsibility here -- the initializer gets an ID from the HTMLWhistElement,
 * and the WhistPlayer gets the ID from the WhistSource.
 */
class WhistSource {
  public:
    WhistSource() : is_whist_(false) {}
    WhistSource(int window_id) : is_whist_(true), window_id_(window_id) {}
    bool IsWhist() const { return is_whist_; }
    int GetWindowId() const { return window_id_; }
  private:
    bool is_whist_;
    int window_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_MEDIASTREAM_WHIST_SOURCE_H_
