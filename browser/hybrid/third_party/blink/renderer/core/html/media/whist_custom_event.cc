/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "whist_custom_event.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_whist_custom_event_init.h"
#include "third_party/blink/renderer/core/event_interface_names.h"

namespace blink {

WhistCustomEvent* WhistCustomEvent::Create() {
  return MakeGarbageCollected<WhistCustomEvent>();
}

WhistCustomEvent::WhistCustomEvent() = default;

WhistCustomEvent::~WhistCustomEvent() = default;

WhistCustomEvent* WhistCustomEvent::Create(const AtomicString& type, const String& value) {
  return MakeGarbageCollected<WhistCustomEvent>(type, value);
}

WhistCustomEvent* WhistCustomEvent::Create(const AtomicString& type,
                                   const WhistCustomEventInit* initializer) {
  return MakeGarbageCollected<WhistCustomEvent>(type, initializer);
}

WhistCustomEvent::WhistCustomEvent(const AtomicString& type,
                           const String& value)
    : Event(type, Bubbles::kNo, Cancelable::kNo),
      value_(value) {}

WhistCustomEvent::WhistCustomEvent(const AtomicString& type,
                           const WhistCustomEventInit* initializer)
    : Event(type, initializer) {
  if (initializer->hasValue())
    value_ = initializer->value();
}

void WhistCustomEvent::initWhistCustomEvent(const AtomicString& type,
                                    bool bubbles,
                                    bool cancelable,
                                    const String& value) {
  if (IsBeingDispatched())
    return;

  initEvent(type, bubbles, cancelable);

  value_ = value;
}

const AtomicString& WhistCustomEvent::InterfaceName() const {
  return event_interface_names::kWhistCustomEvent;
}

void WhistCustomEvent::Trace(Visitor* visitor) const {
  Event::Trace(visitor);
}

}  // namespace blink
