#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_WHIST_CUSTOM_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_WHIST_CUSTOM_EVENT_H_

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class WhistCustomEventInit;

class WhistCustomEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static WhistCustomEvent* Create();
  static WhistCustomEvent* Create(const AtomicString& type, const String& value);
  static WhistCustomEvent* Create(const AtomicString&, const WhistCustomEventInit*);

  WhistCustomEvent();
  WhistCustomEvent(const AtomicString& type, const String& value);
  WhistCustomEvent(const AtomicString&, const WhistCustomEventInit*);
  ~WhistCustomEvent() override;

  const String& value() const { return value_; }

  void initWhistCustomEvent(const AtomicString& type,
                            bool bubbles,
                            bool cancelable,
                            const String& value);

  // From Blink upstream: Needed once we support init<blank>EventNS
  // void initWhistCustomEventNS(in DOMString namespaceURI, in DOMString typeArg,
  //     in boolean canBubbleArg, in boolean cancelableArg, in DOMString valueArg);

  const AtomicString& InterfaceName() const override;

  void Trace(Visitor*) const override;

 private:
  String value_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_WHIST_CUSTOM_EVENT_H_
