#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_WHIST_STRING_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_WHIST_STRING_EVENT_H_

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class WhistStringEventInit;

class WhistStringEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static WhistStringEvent* Create();
  static WhistStringEvent* Create(const AtomicString& type, const String& value);
  static WhistStringEvent* Create(const AtomicString&, const WhistStringEventInit*);

  WhistStringEvent();
  WhistStringEvent(const AtomicString& type, const String& value);
  WhistStringEvent(const AtomicString&, const WhistStringEventInit*);
  ~WhistStringEvent() override;

  const String& value() const { return value_; }

  void initWhistStringEvent(const AtomicString& type,
                            bool bubbles,
                            bool cancelable,
                            const String& value);

  // From Blink upstream: Needed once we support init<blank>EventNS
  // void initWhistStringEventNS(in DOMString namespaceURI, in DOMString typeArg,
  //     in boolean canBubbleArg, in boolean cancelableArg, in DOMString valueArg);

  const AtomicString& InterfaceName() const override;

  void Trace(Visitor*) const override;

 private:
  String value_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_WHIST_STRING_EVENT_H_
