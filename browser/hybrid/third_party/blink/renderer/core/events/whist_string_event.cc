// Copyright 2022 Whist Technologies, Inc. All rights reserved.
#include "whist_string_event.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_whist_string_event_init.h"
#include "third_party/blink/renderer/core/event_interface_names.h"

namespace blink {

WhistStringEvent* WhistStringEvent::Create() {
  return MakeGarbageCollected<WhistStringEvent>();
}

WhistStringEvent::WhistStringEvent() = default;

WhistStringEvent::~WhistStringEvent() = default;

WhistStringEvent* WhistStringEvent::Create(const AtomicString& type, const String& value) {
  return MakeGarbageCollected<WhistStringEvent>(type, value);
}

WhistStringEvent* WhistStringEvent::Create(const AtomicString& type,
                                   const WhistStringEventInit* initializer) {
  return MakeGarbageCollected<WhistStringEvent>(type, initializer);
}

WhistStringEvent::WhistStringEvent(const AtomicString& type,
                           const String& value)
    : Event(type, Bubbles::kNo, Cancelable::kNo),
      value_(value) {}

WhistStringEvent::WhistStringEvent(const AtomicString& type,
                           const WhistStringEventInit* initializer)
    : Event(type, initializer) {
  if (initializer->hasValue())
    value_ = initializer->value();
}

void WhistStringEvent::initWhistStringEvent(const AtomicString& type,
                                    bool bubbles,
                                    bool cancelable,
                                    const String& value) {
  if (IsBeingDispatched())
    return;

  initEvent(type, bubbles, cancelable);

  value_ = value;
}

const AtomicString& WhistStringEvent::InterfaceName() const {
  return event_interface_names::kWhistStringEvent;
}

void WhistStringEvent::Trace(Visitor* visitor) const {
  Event::Trace(visitor);
}

}  // namespace blink
