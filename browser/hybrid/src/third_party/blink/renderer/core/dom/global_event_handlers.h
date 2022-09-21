#ifndef WHIST_HYBRID_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_GLOBAL_EVENT_HANDLERS_H
#define WHIST_HYBRID_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_GLOBAL_EVENT_HANDLERS_H

#define GlobalEventHandlers GlobalEventHandlers_ChromiumImpl
#include "src/third_party/blink/renderer/core/dom/global_event_handlers.h"
#undef GlobalEventHandlers

namespace blink {
  class GlobalEventHandlers : public GlobalEventHandlers_ChromiumImpl {
   public:
    // Register custom events for Whist here after declaring them in whist_event_type_names.json5
    DEFINE_STATIC_ATTRIBUTE_EVENT_LISTENER(whistcustomevent, kWhistcustomevent)
  };
}

#endif  // WHIST_HYBRID_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_GLOBAL_EVENT_HANDLERS_H
