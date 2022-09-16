/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights
 * reserved.
 *
 * Copyright (c) 2022 Whist Technologies Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include "whist/browser/hybrid/third_party/blink/renderer/core/html/media/html_whist_element.h"
#include "whist/browser/hybrid/third_party/blink/renderer/core/events/whist_string_event.h"

#include "third_party/blink/renderer/core/resize_observer/resize_observer_entry.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"

#include "third_party/blink/renderer/core/clipboard/data_transfer.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer_item_list.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer_item.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/native_event_listener.h"
#include "third_party/blink/renderer/core/events/drag_event.h"
#include "third_party/blink/renderer/core/events/gesture_event.h"
#include "third_party/blink/renderer/core/events/keyboard_event.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/wheel_event.h"
#include "third_party/blink/renderer/core/events/focus_event.h"
#include "third_party/blink/renderer/core/input/keyboard_event_manager.h"
#include "third_party/blink/public/mojom/input/focus_type.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/focus_params.h"
#include "whist/browser/hybrid/third_party/whist/protocol_client_interface.h"
#include "third_party/blink/renderer/platform/json/json_parser.h"
#include "third_party/blink/renderer/core/page/pointer_lock_controller.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/json/json_values.h"
#include "third_party/blink/renderer/core/css/css_property_names.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/whist_event_type_names.h"

namespace blink {

using mojom::blink::FileChooserParams;

template<typename...T>
static void WhistElementSequencedTask(HTMLWhistElement* element, void (HTMLWhistElement::*cb)(T...), T... args) {
  PostCrossThreadTask(
      *(element->sequenced_task_runner_), FROM_HERE,
      CrossThreadBindOnce(cb, element->GetWeakPtr(), args...));
}

class HTMLWhistElement::HTMLWhistElementResizeObserverDelegate final
    : public ResizeObserver::Delegate {
 public:
  explicit HTMLWhistElementResizeObserverDelegate(HTMLWhistElement* element)
      : element_(element) {
    DCHECK(element);
  }
  ~HTMLWhistElementResizeObserverDelegate() override = default;
  static const int kDefaultDPI = 96;

  void OnResize(
      const HeapVector<Member<ResizeObserverEntry>>& entries) override {
    // TODO: put something here that passes the size to the player
    DCHECK_EQ(1u, entries.size());
    DCHECK_EQ(entries[0]->target(), element_);

    DOMRectReadOnly* new_size = entries[0]->contentRect();
    float dpr = element_->GetDocument().DevicePixelRatio();
    LOG(INFO) << "HTMLWhistElement OnResize: width: " << new_size->width()
              << " height: " << new_size->height()
              << " DevicePixelRatio: " << element_->GetDocument().DevicePixelRatio();
    element_->CacheResize(new_size->width() * dpr, new_size->height() * dpr, kDefaultDPI * dpr);
    element_->ProcessCachedResize();
  }

  void Trace(Visitor* visitor) const override {
    visitor->Trace(element_);
    ResizeObserver::Delegate::Trace(visitor);
  }

 private:
  Member<HTMLWhistElement> element_;
};

class HTMLWhistElement::WheelEventListener : public NativeEventListener {
 public:
  explicit WheelEventListener(HTMLWhistElement* element) : element_(element) {
    DCHECK(element);
  }
  WheelEventListener(const WheelEventListener&) = delete;
  WheelEventListener& operator=(const WheelEventListener&) = delete;
  ~WheelEventListener() override = default;
  void StartListening() {
    element_->addEventListener(event_type_names::kWheel, this, false);
  }

  void Trace(Visitor* visitor) const override {
    NativeEventListener::Trace(visitor);
    visitor->Trace(element_);
  }

 private:
  void Invoke(ExecutionContext*, Event* event) override {
    auto* wheel_event = DynamicTo<WheelEvent>(event);
    WhistClient::WhistFrontendEvent frontend_event;
    frontend_event.type = WhistClient::FRONTEND_EVENT_MOUSE_WHEEL;

    if (wheel_event->NativeEvent().momentum_phase == WebMouseWheelEvent::Phase::kPhaseBegan)
      frontend_event.mouse_wheel.momentum_phase = WhistClient::MOUSEWHEEL_MOMENTUM_BEGIN;
    else if (wheel_event->NativeEvent().momentum_phase == WebMouseWheelEvent::Phase::kPhaseChanged)
      frontend_event.mouse_wheel.momentum_phase = WhistClient::MOUSEWHEEL_MOMENTUM_ACTIVE;
    else if (wheel_event->NativeEvent().momentum_phase == WebMouseWheelEvent::Phase::kPhaseEnded)
      frontend_event.mouse_wheel.momentum_phase = WhistClient::MOUSEWHEEL_MOMENTUM_END;
    else
      frontend_event.mouse_wheel.momentum_phase = WhistClient::MOUSEWHEEL_MOMENTUM_NONE;

    frontend_event.mouse_wheel.precise_delta.x =
        (float)wheel_event->deltaX() / WheelEvent::kTickMultiplier;
    frontend_event.mouse_wheel.precise_delta.y =
        -(float)wheel_event->deltaY() / WheelEvent::kTickMultiplier;

    // For DeltaX, +1 is scroll right, -1 is scroll left
    if ((int)wheel_event->deltaX() > 0) {
      frontend_event.mouse_wheel.delta.x = 1;
    } else if ((int)wheel_event->deltaX() < 0) {
      frontend_event.mouse_wheel.delta.x = -1;
    } else {
      frontend_event.mouse_wheel.delta.x = 0;
    }

    // For DeltaY, +1 is scroll up, -1 is scroll down
    if ((int)wheel_event->deltaY() > 0) {
      frontend_event.mouse_wheel.delta.y = -1;
    } else if ((int)wheel_event->deltaY() < 0) {
      frontend_event.mouse_wheel.delta.y = 1;
    } else {
      frontend_event.mouse_wheel.delta.y = 0;
    }
    WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
  }

  Member<HTMLWhistElement> element_;
};

HTMLWhistElement::HTMLWhistElement(Document& document)
    : HTMLVideoElement(html_names::kWhistTag, document),
      sequenced_task_runner_(base::SequencedTaskRunnerHandle::Get()),
      resize_observer_(ResizeObserver::Create(
          GetDocument().domWindow(),
          MakeGarbageCollected<HTMLWhistElementResizeObserverDelegate>(this))),
      wheel_event_listener_(
          MakeGarbageCollected<WheelEventListener>(this)),
      whist_window_id_(WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.create_window))
{
  resize_observer_->observe(this);
  wheel_event_listener_->StartListening();

  // Create a whist window for this htmlwhistelement
  WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.register_context, whist_window_id_, this);

  // Set input callbacks

  WHIST_VIRTUAL_INTERFACE_CALL(file.set_on_file_upload_callback, whist_window_id_, [](void* ctx) -> const char* {
    HTMLWhistElement* element = (HTMLWhistElement*)ctx;
    WhistElementSequencedTask(element, &HTMLWhistElement::OpenFileChooser);
    return element->GetChosenFile();
  });

  WHIST_VIRTUAL_INTERFACE_CALL(video.set_on_cursor_change_callback, whist_window_id_, [](void* ctx, const char* cursor_type, bool relative_mouse_mode) {
    WhistElementSequencedTask((HTMLWhistElement*)ctx, &HTMLWhistElement::UpdateCursorType, String(cursor_type), relative_mouse_mode);
  });

  WHIST_VIRTUAL_INTERFACE_CALL(events.set_get_modifier_key_state, []() -> int {
    int modifier_flags = 0;
#ifdef _WIN32
    // Use WinAPI functions to obtain keystate
#define HIGHBITMASKSHORT 0x8000
    if (GetKeyState(VK_SHIFT) & HIGHBITMASKSHORT) {
      modifier_flags |= WhistClient::SHIFT;
    }
    if (GetKeyState(VK_CONTROL) & HIGHBITMASKSHORT) {
      modifier_flags |= WhistClient::CTRL;
    }
    if (GetKeyState(VK_MENU) & HIGHBITMASKSHORT) {
      modifier_flags |= WhistClient::ALT;
    }
    if ((GetKeyState(VK_LWIN) & HIGHBITMASKSHORT) || (GetKeyState(VK_RWIN) & HIGHBITMASKSHORT)) {
      modifier_flags |= WhistClient::GUI;
    }
#else
    // Use chromium implementation, which only works on MacOSX
    WebInputEvent::Modifiers modifiers =
        KeyboardEventManager::GetCurrentModifierState();
    if (modifiers & WebInputEvent::kShiftKey) {
      modifier_flags |= WhistClient::SHIFT;
    }
    if (modifiers & WebInputEvent::kControlKey) {
      modifier_flags |= WhistClient::CTRL;
    }
    if (modifiers & WebInputEvent::kAltKey) {
      modifier_flags |= WhistClient::ALT;
    }
    if (modifiers & WebInputEvent::kMetaKey) {
      modifier_flags |= WhistClient::GUI;
    }
    if (KeyboardEventManager::CurrentCapsLockState()) {
      modifier_flags |= WhistClient::CAPS;
    }
#endif
    return modifier_flags;
  });

  // Uncomment this if we want the Whist element to have focus as soon as it is created:
  // GetDocument().SetFocusedElement(
  //   this, FocusParams(SelectionBehaviorOnFocus::kRestore,
  //                     mojom::blink::FocusType::kNone, nullptr));
}

HTMLWhistElement::~HTMLWhistElement() {
  // lifecycle.destroy_window is called in the WhistPlayer's FrameDeliverer destructor.
}

void HTMLWhistElement::Trace(Visitor* visitor) const {
  Supplementable<HTMLWhistElement>::Trace(visitor);
  HTMLVideoElement::Trace(visitor);
  visitor->Trace(resize_observer_);
  visitor->Trace(wheel_event_listener_);
}

void HTMLWhistElement::DefaultEventHandler(Event& event) {
  // This group_id has to be globally monotonic across all instances of
  // HTMLWhistElement. Hence not making this as member variable.
  static int group_id = 0;

  // Reference blink/renderer/core/dom/node.cc to see how the default handler behaves
  if (event.target() != this)
    return;

  // Reference blink/renderer/core/events/event_type_names.json5 to see all the different event types
  const AtomicString& event_type = event.type();
  if (event_type == event_type_names::kKeydown ||
    event_type == event_type_names::kKeyup) {
    auto* keyboard_event = DynamicTo<KeyboardEvent>(&event);
    WhistClient::WhistFrontendEvent frontend_event;
    frontend_event.type = WhistClient::FRONTEND_EVENT_KEYPRESS;
    frontend_event.keypress.code = (WhistClient::WhistKeycode)(keyboard_event->KeyEvent()->dom_code & 0xFFFF);
    // mod is anyways ignored by the server. No use in populating this value.
    // Guess keyboard state message is used by the server to sync the state of modifier keys.
    frontend_event.keypress.mod = WhistClient::MOD_NONE;
    frontend_event.keypress.pressed = event_type == event_type_names::kKeydown ? true : false;
    WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
    // Stateful keys namely CapsLock, NumLock and ScrollLock needs to be handled
    // both locally and in cloud server. For all other keys, the handling is
    // done only in cloud server. This will ensure other browser Shortcuts such
    // "zoom" shortcut to be processed only in cloud and not in local browser.
    if (frontend_event.keypress.code != WhistClient::FK_SCROLLLOCK &&
        frontend_event.keypress.code != WhistClient::FK_NUMLOCK &&
        frontend_event.keypress.code != WhistClient::FK_CAPSLOCK) {
      keyboard_event->SetDefaultHandled();
    }
  } else if (event_type == event_type_names::kMousemove) {
    auto* mouse_event = DynamicTo<MouseEvent>(&event);
    float dpr = GetDocument().DevicePixelRatio();
    WhistClient::WhistFrontendEvent frontend_event;
    frontend_event.type = WhistClient::FRONTEND_EVENT_MOUSE_MOTION;
    if (GetDocument().GetPage()->GetPointerLockController().IsPointerLocked()) {
      frontend_event.mouse_motion.relative.x = (int)mouse_event->movementX() * dpr;
      frontend_event.mouse_motion.relative.y = (int)mouse_event->movementY() * dpr;
      frontend_event.mouse_motion.relative_mode = true;
    } else {
      frontend_event.mouse_motion.absolute.x = (int)mouse_event->clientX() * dpr;
      frontend_event.mouse_motion.absolute.y = (int)mouse_event->clientY() * dpr;
      frontend_event.mouse_motion.relative_mode = false;
    }
    WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
  } else if (event_type == event_type_names::kMousedown ||
             event_type == event_type_names::kMouseup) {
    auto* mouse_event = DynamicTo<MouseEvent>(&event);
    // Switch focus to Whist element as soon as it is clicked
    GetDocument().SetFocusedElement(
      this, FocusParams(SelectionBehaviorOnFocus::kRestore,
                        mojom::blink::FocusType::kNone, nullptr));

    WhistClient::WhistFrontendEvent frontend_event;
    frontend_event.type = WhistClient::FRONTEND_EVENT_MOUSE_BUTTON;
    if (mouse_event->button() == static_cast<int>(WebPointerProperties::Button::kLeft)) {
      frontend_event.mouse_button.button = WhistClient::MOUSE_L;
    } else if (mouse_event->button() == static_cast<int>(WebPointerProperties::Button::kMiddle)) {
      frontend_event.mouse_button.button = WhistClient::MOUSE_MIDDLE;
    } else if (mouse_event->button() == static_cast<int>(WebPointerProperties::Button::kRight)) {
      frontend_event.mouse_button.button = WhistClient::MOUSE_R;
    } else if (mouse_event->button() == static_cast<int>(WebPointerProperties::Button::kBack)) {
      frontend_event.mouse_button.button = WhistClient::MOUSE_X1;
    } else if (mouse_event->button() == static_cast<int>(WebPointerProperties::Button::kForward)) {
      frontend_event.mouse_button.button = WhistClient::MOUSE_X2;
    } else {
      LOG(ERROR) << "Unmapped button value : " << mouse_event->button();
      return;
    }
    frontend_event.mouse_button.pressed = event_type == event_type_names::kMousedown ? true : false;
    WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
    mouse_event->SetDefaultHandled();
  } else if (event_type == event_type_names::kDragenter) {
    auto* drag_event = DynamicTo<DragEvent>(&event);
    DataTransferItemList* item_list = drag_event->getDataTransfer()->items();
    group_id++;
    for (uint32_t i = 0; i < item_list->length(); i++) {
      SendFileDragEvent(item_list->item(i)
                                   ->GetDataObjectItem()
                                   ->GetAsFile()
                                   ->GetPath()
                                   .Utf8(),
                        false, group_id, drag_event);
    }
    drag_event->SetDefaultHandled();
  } else if (event_type == event_type_names::kDragover) {
    auto* drag_event = DynamicTo<DragEvent>(&event);
    SendFileDragEvent({}, false, group_id, drag_event);
    drag_event->SetDefaultHandled();
  } else if (event_type == event_type_names::kDragleave) {
    auto* drag_event = DynamicTo<DragEvent>(&event);
    SendFileDragEvent({}, true, group_id, drag_event);
    drag_event->SetDefaultHandled();
  } else if (event_type == event_type_names::kDrop) {
    auto* drag_event = DynamicTo<DragEvent>(&event);
    SendFileDragEvent({}, true, group_id, drag_event);
    DataTransferItemList* item_list = drag_event->getDataTransfer()->items();
    for (uint32_t i = 0; i < item_list->length(); i++) {
      SendFileDropEvent(item_list->item(i)
                                   ->GetDataObjectItem()
                                   ->GetAsFile()
                                   ->GetPath()
                                   .Utf8(),
                        false, drag_event);
    }
    SendFileDropEvent({}, true, drag_event);
    drag_event->SetDefaultHandled();
  } else if (event_type == event_type_names::kFocus || event_type == event_type_names::kBlur) {
    // Stub for now -- we might instead just use whistPause() and whistPlay from the WebUI
    // Javascript to manage the "pause on blur, play on focus, most-recently-active tab"
    // functionality.
  } else if (event_type == whist_event_type_names::kGesturepinchbegin) {
    // Pinch Begin does not have an WhistFrontEnd equivalent. So just mark is as handled.
    event.SetDefaultHandled();
  } else if (event_type == whist_event_type_names::kGesturepinchupdate ||
             event_type == whist_event_type_names::kGesturepinchend) {
      auto* gesture_event = DynamicTo<GestureEvent>(&event);
      WhistClient::WhistFrontendEvent frontend_event;
      frontend_event.type = WhistClient::FRONTEND_EVENT_GESTURE;
      frontend_event.gesture.delta.theta = 0.0;
      frontend_event.gesture.center.x = frontend_event.gesture.center.y = 0.0;
      frontend_event.gesture.num_fingers = 2;
      if (event_type == whist_event_type_names::kGesturepinchupdate) {
        // This ratio is derived empirically based on the pinch to zoom tests done in
        // native tab vs cloud tab.
        // TODO : If possible, find a systematic way to convert chromium scale value to
        // whist delta distance.
        #define SCALE_TO_DIST_RATIO 5.0
        frontend_event.gesture.delta.dist =
            (gesture_event->NativeEvent().data.pinch_update.scale - 1.0) *
            SCALE_TO_DIST_RATIO;
        if (gesture_event->NativeEvent().data.pinch_update.scale > 1.0)
          frontend_event.gesture.type = WhistClient::MULTIGESTURE_PINCH_OPEN;
        else
          frontend_event.gesture.type = WhistClient::MULTIGESTURE_PINCH_CLOSE;
      } else {
        frontend_event.gesture.delta.dist = 0;
        frontend_event.gesture.type = WhistClient::MULTIGESTURE_NONE;
      }
    WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
    event.SetDefaultHandled();
  }
}

int HTMLWhistElement::freezeAll() {
  return WHIST_VIRTUAL_INTERFACE_CALL(video.freeze_all_windows);
}

void HTMLWhistElement::requestSpotlight(int spotlight_id) {
  ProcessCachedResize();
  WHIST_VIRTUAL_INTERFACE_CALL(video.set_video_spotlight, whist_window_id_, spotlight_id);
}

bool HTMLWhistElement::isWhistConnected() {
  bool is_whist_connected = WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.is_connected);
  return is_whist_connected;
}

void HTMLWhistElement::whistConnect(const String& whist_parameters) {
  // Initiate a new whist connection
  bool new_connection = WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.connect);

  // Send parameters for this new whist connection
  if (new_connection) {
    // IMPORTANT: Calling lifecycle.connect() flushes the event queue, so we must re-submit
    // the resize event from element startup. This is to ensure that the protocol has the
    // correct dimensions and DPI at startup.
    ProcessCachedResize();

    WhistClient::WhistFrontendEvent event = {};
    event.type = WhistClient::FRONTEND_EVENT_STARTUP_PARAMETER;
    event.startup_parameter.error = false;
    // TODO: Validate that this is actually a valid JSONObject so that we don't die on bad parameter values..
    auto json_object = JSONObject::From(ParseJSON(whist_parameters));
    for (wtf_size_t i = 0; i < json_object->size(); ++i) {
      JSONObject::Entry entry = json_object->at(i);
      String wtf_key = entry.first;
      String wtf_value;
      entry.second->AsString(&wtf_value);

      // Get key/value as std::string
      std::string str_key = wtf_key.Utf8();
      std::string str_value = wtf_value.Utf8();

      // Allocate and populate the whist-owned strings
      event.startup_parameter.key = (char*)WHIST_VIRTUAL_INTERFACE_CALL(utils.malloc, str_key.size() + 1);
      event.startup_parameter.value = (char*)WHIST_VIRTUAL_INTERFACE_CALL(utils.malloc, str_value.size() + 1);
      strcpy(event.startup_parameter.key, str_key.c_str());
      strcpy(event.startup_parameter.value, str_value.c_str());

      // key and value are freed by the WhistClient event handler
      WHIST_VIRTUAL_INTERFACE_CALL(events.send, &event);
    }

    // Mark as finished, so that whist may connect
    event.startup_parameter.key = (char*)WHIST_VIRTUAL_INTERFACE_CALL(utils.malloc, strlen("finished") + 1);
    strcpy(event.startup_parameter.key, "finished");
    event.startup_parameter.value = NULL;
    WHIST_VIRTUAL_INTERFACE_CALL(events.send, &event);
  }
}

Node::InsertionNotificationRequest HTMLWhistElement::InsertedInto(
    ContainerNode& insertion_point) {
  SetSrcObject(nullptr); // placeholder for launching the functions that create the whistplayer

  if (!resize_observer_) {
    resize_observer_ = ResizeObserver::Create(
        GetDocument().domWindow(),
        MakeGarbageCollected<HTMLWhistElementResizeObserverDelegate>(this));
    resize_observer_->observe(this);
  }

  return HTMLVideoElement::InsertedInto(insertion_point);
}

void HTMLWhistElement::RemovedFrom(ContainerNode& insertion_point) {
  if (resize_observer_) {
    resize_observer_->disconnect();
    resize_observer_.Clear();
  }

  HTMLVideoElement::RemovedFrom(insertion_point);
}

void HTMLWhistElement::ParseAttribute(
    const AttributeModificationParams& params) {
}

void HTMLWhistElement::OpenFileChooser() {
#if defined(WIN32)
  chosen_file_ = L"";
#else
  chosen_file_ = "";
#endif
  if (ChromeClient* chrome_client = GetChromeClient()) {
    Document& document = GetDocument();
    FileChooserParams params;
    params.mode = FileChooserParams::Mode::kOpen;
    params.title = g_empty_string;
    params.need_local_path = false;
    params.requestor = document.Url();

    UseCounter::Count(
        document, GetExecutionContext()->IsSecureContext()
                      ? WebFeature::kInputTypeFileSecureOriginOpenChooser
                      : WebFeature::kInputTypeFileInsecureOriginOpenChooser);
    chrome_client->OpenFileChooser(document.GetFrame(), NewFileChooser(params));
  } else {
    waitable_event_.Signal();
  }

  return;
}

// Warning: On Windows, Blink uses std::wstring, so converting wstring to char* may lead to issues.
// As a result, we may need to rewrite GetChosenFile() for Windows.
const char* HTMLWhistElement::GetChosenFile() {
  base::TimeDelta timeout = base::Milliseconds(50);
  while (!waitable_event_.TimedWait(timeout)) {
    FileChooser *file_chooser = FileChooserOrNull();
    // Handle the case where user could have closed the file chooser popup
    // without selecting any file
    if (!file_chooser || !file_chooser->FrameOrNull())
      break;
  }
  waitable_event_.Reset();
  LOG(INFO) << "Returning chosen file to Whist " << chosen_file_;
#if defined(WIN32)
  if (chosen_file_ == L"")
    return NULL;
  else
  {
    return (char*)chosen_file_.c_str();
  }
#else
  if (chosen_file_ == "")
    return NULL;
  else
    return chosen_file_.c_str();
#endif
}

void HTMLWhistElement::UpdateCursorType(String type, bool relative_mouse_mode) {
    // Update the cursor type
    SetInlineStyleProperty(CSSPropertyID::kCursor, type);
    // Try to get the document page
    Page* page = GetDocument().GetPage();
    if (page == nullptr) {
      return;
    }
    // Update cursor mouse mode
    bool locked = page->GetPointerLockController().IsPointerLocked();
    if (locked && !relative_mouse_mode) {
      page->GetPointerLockController().ExitPointerLock();
    } else if (!locked && relative_mouse_mode) {
      page->GetPointerLockController().RequestPointerLock(this, base::BindOnce([](mojom::blink::PointerLockResult result) {}));
    }
}

void HTMLWhistElement::FilesChosen(FileChooserFileInfoList files,
                                const base::FilePath& base_dir) {
  for (wtf_size_t i = 0; i < files.size();) {
    // Drop files of which names can not be converted to WTF String. We
    // can't expose such files via File API.
    if (files[i]->is_native_file() &&
        FilePathToString(files[i]->get_native_file()->file_path).IsEmpty()) {
      files.EraseAt(i);
      // Do not increment |i|.
      continue;
    }
    chosen_file_ = files[i]->get_native_file()->file_path.value();
    break;
  }

  if (HasConnectedFileChooser())
    DisconnectFileChooser();
  waitable_event_.Signal();
}

LocalFrame* HTMLWhistElement::FrameOrNull() const {
  return GetDocument().GetFrame();
}

ChromeClient* HTMLWhistElement::GetChromeClient() const {
  if (Page* page = GetDocument().GetPage())
    return &page->GetChromeClient();
  return nullptr;
}

void HTMLWhistElement::WillOpenPopup() {
  // TODO(tkent): Should we disconnect the file chooser? crbug.com/637639
  if (HasConnectedFileChooser()) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kPopupOpenWhileFileChooserOpened);
  }
}

void HTMLWhistElement::SendFileDragEvent(std::optional<std::string> file_name,
                                         bool end_drag,
                                         int group_id,
                                         DragEvent* drag_event) {
  WhistClient::WhistFrontendEvent frontend_event = {};
  frontend_event.type = WhistClient::FRONTEND_EVENT_FILE_DRAG;
  if (file_name.has_value()) {
    frontend_event.file_drag.filename = (char*)WHIST_VIRTUAL_INTERFACE_CALL(utils.malloc, file_name->size() + 1);
    strcpy(frontend_event.file_drag.filename, file_name->c_str());
  } else {
    frontend_event.file_drag.filename = NULL;
  }
  frontend_event.file_drag.end_drag = end_drag;
  frontend_event.file_drag.group_id = group_id;
  frontend_event.file_drag.position.x = (int)drag_event->clientX();
  frontend_event.file_drag.position.y = (int)drag_event->clientY();
  WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
}

void HTMLWhistElement::SendFileDropEvent(std::optional<std::string> file_name,
                                         bool end_drop,
                                         DragEvent* drag_event) {
  WhistClient::WhistFrontendEvent frontend_event = {};
  frontend_event.type = WhistClient::FRONTEND_EVENT_FILE_DROP;
  if (file_name.has_value()) {
    frontend_event.file_drop.filename = (char*)WHIST_VIRTUAL_INTERFACE_CALL(utils.malloc, file_name->size() + 1);
    strcpy(frontend_event.file_drop.filename, file_name->c_str());
  } else {
    frontend_event.file_drop.filename = NULL;
  }
  frontend_event.file_drop.end_drop = end_drop;
  frontend_event.file_drop.position.x = (int)drag_event->clientX();
  frontend_event.file_drop.position.y = (int)drag_event->clientY();
  WHIST_VIRTUAL_INTERFACE_CALL(events.send, &frontend_event);
}

void HTMLWhistElement::CacheResize(int width, int height, int dpi) {
  cached_resize_.width = width;
  cached_resize_.height = height;
  cached_resize_.dpi = dpi;
  cached_resize_.set = true;
}

void HTMLWhistElement::ProcessCachedResize() {
  if (!cached_resize_.set) return;
  WhistClient::WhistFrontendEvent event;
  event.type = WhistClient::FRONTEND_EVENT_RESIZE;
  event.resize.width = cached_resize_.width;
  event.resize.height = cached_resize_.height;
  event.resize.dpi = cached_resize_.dpi;
  WHIST_VIRTUAL_INTERFACE_CALL(events.send, &event);
}

} // namespace blink
