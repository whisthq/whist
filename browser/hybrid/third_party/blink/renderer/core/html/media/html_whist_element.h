/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights
 * reserved.
 *
 * Coypright (C) 2022 Whist Technologies, Inc. All rights reserved.
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

#ifndef WHIST_BROWSER_HYBRID_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_WHIST_ELEMENT_H_
#define WHIST_BROWSER_HYBRID_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_WHIST_ELEMENT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer.h"
#include "third_party/blink/renderer/core/html/forms/file_chooser.h"
#include "base/memory/weak_ptr.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier_media.h"

namespace blink {

class DragEvent;

class CORE_EXPORT HTMLWhistElement final
    : public HTMLVideoElement,
      public Supplementable<HTMLWhistElement>,
      private FileChooserClient {
  DEFINE_WRAPPERTYPEINFO();

  public:
    explicit HTMLWhistElement(Document&);
    HTMLWhistElement(const HTMLWhistElement&) = delete;
    HTMLWhistElement& operator=(const HTMLWhistElement&) = delete;
    ~HTMLWhistElement() override;
    void Trace(Visitor* visitor) const override;

    bool IsHTMLWhistElement() const final { return true; }
    int GetWhistWindowId() const final { return whist_window_id_; }
    bool IsKeyboardFocusable() const override { return true; }
    // TODO: There's some weird logic in layout_theme.cc that makes the below not quite work
    // (there's still a blue ring around the whist tag when focused.)
    bool ShouldHaveFocusAppearance() const override { return false; }

    void ParseAttribute(const AttributeModificationParams& params) override;

    // Node override.
    void DefaultEventHandler(Event& event) override;
    Node::InsertionNotificationRequest InsertedInto(ContainerNode&) override;
    void RemovedFrom(ContainerNode& insertion_point) override;
    void OpenFileChooser();
    void UpdateCursorType(String type, bool relative_mouse_mode);
    auto GetWeakPtr() { return weak_ptr_factory_.GetWeakPtr(); }
    const char* GetChosenFile();
    const scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner_;
    void CacheResize(int width, int height, int dpi);
    void ProcessCachedResize();

    // These functions are called from JavaScript -- see html_whist_element.idl for bindings.
    unsigned int freezeAll();
    void requestSpotlight(int spotlight_id);
    bool isWhistConnected();
    void whistConnect(const String& whist_parameters);

  private:
    class HTMLWhistElementResizeObserverDelegate;
    class WheelEventListener;
    void SendFileDragEvent(std::optional<std::string> file_name,
                           bool end_drag,
                           int group_id,
                           DragEvent* drag_event);
    void SendFileDropEvent(std::optional<std::string> file_name,
                           bool end_drop,
                           DragEvent* drag_event);
    // FileChooserClient implementation.
    void FilesChosen(FileChooserFileInfoList files,
                     const base::FilePath& base_dir) override;
    LocalFrame* FrameOrNull() const override;
    ChromeClient* GetChromeClient() const;

    // PopupOpeningObserver implementation.
    void WillOpenPopup() override;

    Member<ResizeObserver> resize_observer_;
    Member<WheelEventListener> wheel_event_listener_;
    int whist_window_id_;
    struct {
      int width;
      int height;
      int dpi;
      bool set;
    } cached_resize_;
    base::WaitableEvent waitable_event_;
    base::FilePath::StringType chosen_file_;
    base::WeakPtrFactory<HTMLWhistElement> weak_ptr_factory_{this};
};

}  // namespace blink

#endif  // WHIST_BROWSER_HYBRID_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_WHIST_ELEMENT_H_
