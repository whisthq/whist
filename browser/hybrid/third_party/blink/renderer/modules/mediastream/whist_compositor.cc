// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "whist/browser/hybrid/third_party/blink/renderer/modules/mediastream/whist_compositor.h"

#include <stdint.h>
#include <string>
#include <utility>

#include "base/hash/hash.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/bind_post_task.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "cc/paint/skia_paint_canvas.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "media/renderers/paint_canvas_video_renderer.h"
#include "services/viz/public/cpp/gpu/context_provider_command_buffer.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_video_frame_submitter.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_component.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_descriptor.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "third_party/libyuv/include/libyuv/video_common.h"
#include "third_party/skia/include/core/SkSurface.h"

#include "whist/browser/hybrid/third_party/blink/public/web/modules/mediastream/whist_player.h"

namespace WTF {

template <typename T>
struct CrossThreadCopier<absl::optional<T>>
    : public CrossThreadCopierPassThrough<absl::optional<T>> {
  STATIC_ONLY(CrossThreadCopier);
};

}  // namespace WTF

namespace blink {

namespace {

gfx::Size RotationAdjustedSize(media::VideoRotation rotation,
                               const gfx::Size& size) {
  if (rotation == media::VIDEO_ROTATION_90 ||
      rotation == media::VIDEO_ROTATION_270) {
    return gfx::Size(size.height(), size.width());
  }

  return size;
}

}  // anonymous namespace

WhistCompositor::WhistCompositor(
    scoped_refptr<base::SingleThreadTaskRunner>
        video_frame_compositor_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    std::unique_ptr<WebVideoFrameSubmitter> submitter,
    WebMediaPlayer::SurfaceLayerMode surface_layer_mode,
    const base::WeakPtr<WhistPlayer>& player)
    : video_frame_compositor_task_runner_(video_frame_compositor_task_runner),
      io_task_runner_(io_task_runner),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      player_(player),
      video_frame_provider_client_(nullptr),
      last_render_length_(base::Seconds(1.0 / 60.0)),
      total_frame_count_(0),
      dropped_frame_count_(0),
      stopped_(true),
      render_started_(!stopped_) {
  if (surface_layer_mode != WebMediaPlayer::SurfaceLayerMode::kNever) {
    submitter_ = std::move(submitter);

    PostCrossThreadTask(
        *video_frame_compositor_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistCompositor::InitializeSubmitter,
                            weak_ptr_factory_.GetWeakPtr()));
    update_submission_state_callback_ = base::BindPostTask(
        video_frame_compositor_task_runner_,
        ConvertToBaseRepeatingCallback(CrossThreadBindRepeating(
            &WhistCompositor::SetIsSurfaceVisible,
            weak_ptr_factory_.GetWeakPtr())));
  }
}

WhistCompositor::~WhistCompositor() {
  // Ensured by destructor traits.
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());

  if (submitter_) {
    video_frame_compositor_task_runner_->DeleteSoon(FROM_HERE,
                                                    std::move(submitter_));
  } else {
    DCHECK(!video_frame_provider_client_)
        << "Must call StopUsingProvider() before dtor!";
  }
}

// static
void WhistCompositorTraits::Destruct(
    const WhistCompositor* compositor) {
  if (!compositor->video_frame_compositor_task_runner_
           ->BelongsToCurrentThread()) {
    PostCrossThreadTask(
        *compositor->video_frame_compositor_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistCompositorTraits::Destruct,
                            CrossThreadUnretained(compositor)));
    return;
  }
  delete compositor;
}

void WhistCompositor::InitializeSubmitter() {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  submitter_->Initialize(this, /* is_media_stream = */ true);
}

void WhistCompositor::SetIsSurfaceVisible(
    bool state,
    base::WaitableEvent* done_event) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  submitter_->SetIsSurfaceVisible(state);
  if (done_event)
    done_event->Signal();
}

// TODO(https://crbug/879424): Rename, since it really doesn't enable
// submission. Do this along with the VideoFrameSubmitter refactor.
void WhistCompositor::EnableSubmission(
    const viz::SurfaceId& id,
    media::VideoTransformation transformation,
    bool force_submit) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());

  // If we're switching to |submitter_| from some other client, then tell it.
  // if (video_frame_provider_client_ &&
  //     video_frame_provider_client_ != submitter_.get()) {
  //   video_frame_provider_client_->StopUsingProvider();
  // }

  submitter_->SetTransform(transformation);
  submitter_->SetForceSubmit(force_submit);
  submitter_->EnableSubmission(id);
  // base::debug::StackTrace().Print();
  video_frame_provider_client_ = submitter_.get();

  // if (!stopped_)
  //   video_frame_provider_client_->StartRendering();
}

void WhistCompositor::SetForceBeginFrames(bool enable) {
  if (!submitter_)
    return;

  if (!video_frame_compositor_task_runner_->BelongsToCurrentThread()) {
    PostCrossThreadTask(
        *video_frame_compositor_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistCompositor::SetForceBeginFrames,
                            weak_ptr_factory_.GetWeakPtr(), enable));
    return;
  }

  submitter_->SetForceBeginFrames(enable);
}

void WhistCompositor::SetForceSubmit(bool force_submit) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  submitter_->SetForceSubmit(force_submit);
}

void WhistCompositor::SetIsPageVisible(bool is_visible) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  if (submitter_)
    submitter_->SetIsPageVisible(is_visible);
}

gfx::Size WhistCompositor::GetCurrentSize() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::AutoLock auto_lock(current_frame_lock_);
  return current_frame_ ? current_frame_->natural_size() : gfx::Size(500, 500);;
}

base::TimeDelta WhistCompositor::GetCurrentTime() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::AutoLock auto_lock(current_frame_lock_);
  return current_frame_.get() ? current_frame_->timestamp() : base::TimeDelta();
}

size_t WhistCompositor::total_frame_count() {
  base::AutoLock auto_lock(current_frame_lock_);
  DVLOG(1) << __func__ << ", " << total_frame_count_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return total_frame_count_;
}

size_t WhistCompositor::dropped_frame_count() {
  base::AutoLock auto_lock(current_frame_lock_);
  DVLOG(1) << __func__ << ", " << dropped_frame_count_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return dropped_frame_count_;
}

void WhistCompositor::SetVideoFrameProviderClient(
    cc::VideoFrameProvider::Client* client) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());

  // base::debug::StackTrace().Print();
  // if (video_frame_provider_client_)
  //   video_frame_provider_client_->StopUsingProvider();

  // video_frame_provider_client_ = client;
  // if (video_frame_provider_client_ && !stopped_)
  //   video_frame_provider_client_->StartRendering();
}

void WhistCompositor::EnqueueFrame(
    scoped_refptr<media::VideoFrame> frame,
    bool is_copy) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  base::AutoLock auto_lock(current_frame_lock_);
  TRACE_EVENT_INSTANT1("media", "WhistCompositor::EnqueueFrame",
                       TRACE_EVENT_SCOPE_THREAD, "Timestamp",
                       frame->timestamp().InMicroseconds());
  ++total_frame_count_;

  RenderWithoutAlgorithm(std::move(frame), is_copy);
  return;
}

bool WhistCompositor::UpdateCurrentFrame(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  return false;
}

bool WhistCompositor::HasCurrentFrame() {
  base::AutoLock auto_lock(current_frame_lock_);
  return !!current_frame_;
}

scoped_refptr<media::VideoFrame> WhistCompositor::GetCurrentFrame() {
  DVLOG(3) << __func__;
  base::AutoLock auto_lock(current_frame_lock_);
  TRACE_EVENT_INSTANT1("media", "WhistCompositor::GetCurrentFrame",
                       TRACE_EVENT_SCOPE_THREAD, "Timestamp",
                       current_frame_->timestamp().InMicroseconds());
  if (!render_started_)
    return nullptr;

  return current_frame_;
}

void WhistCompositor::PutCurrentFrame() {
  DVLOG(3) << __func__;
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
}

base::TimeDelta WhistCompositor::GetPreferredRenderInterval() {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());

  DCHECK_GE(last_render_length_, base::TimeDelta());
  return last_render_length_;
}

void WhistCompositor::StartRendering() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  LOG(ERROR) << "WhistCompositor::StartRendering";
  {
    base::AutoLock auto_lock(current_frame_lock_);
    render_started_ = true;
  }
  PostCrossThreadTask(
      *video_frame_compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(&WhistCompositor::StartRenderingInternal,
                          WrapRefCounted(this)));
}

void WhistCompositor::StopRendering() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  PostCrossThreadTask(
      *video_frame_compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(&WhistCompositor::StopRenderingInternal,
                          WrapRefCounted(this)));
}

void WhistCompositor::StopUsingProvider() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  PostCrossThreadTask(
      *video_frame_compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(
          &WhistCompositor::StopUsingProviderInternal,
          WrapRefCounted(this)));
}

void WhistCompositor::RenderWithoutAlgorithm(
    scoped_refptr<media::VideoFrame> frame,
    bool is_copy) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  PostCrossThreadTask(
      *video_frame_compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(
          &WhistCompositor::RenderWithoutAlgorithmOnCompositor,
          WrapRefCounted(this), std::move(frame), is_copy));
}

void WhistCompositor::RenderWithoutAlgorithmOnCompositor(
    scoped_refptr<media::VideoFrame> frame,
    bool is_copy) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  {
    base::AutoLock auto_lock(current_frame_lock_);
    // Last timestamp in the stream might not have timestamp.
    if (current_frame_ && !frame->timestamp().is_zero() &&
        frame->timestamp() > current_frame_->timestamp()) {
      last_render_length_ = frame->timestamp() - current_frame_->timestamp();
    }
    SetCurrentFrame(std::move(frame), is_copy, absl::nullopt);
  }
  if (video_frame_provider_client_)
    video_frame_provider_client_->DidReceiveFrame();
}

void WhistCompositor::SetCurrentFrame(
    scoped_refptr<media::VideoFrame> frame,
    bool is_copy,
    absl::optional<base::TimeTicks> expected_display_time) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  current_frame_lock_.AssertAcquired();
  TRACE_EVENT_INSTANT1("media", "WhistCompositor::SetCurrentFrame",
                       TRACE_EVENT_SCOPE_THREAD, "Timestamp",
                       frame->timestamp().InMicroseconds());

  // Compare current frame with |frame|. Initialize values as if there is no
  // current frame.
  bool is_first_frame = true;
  bool has_frame_size_changed = false;

  absl::optional<media::VideoTransformation> new_transform =
      media::kNoTransformation;
  if (frame->metadata().transformation)
    new_transform = frame->metadata().transformation;

  absl::optional<bool> new_opacity;
  new_opacity = media::IsOpaque(frame->format());

  if (current_frame_) {
    // We have a current frame, so determine what has changed.
    is_first_frame = false;

    auto current_video_transform =
        current_frame_->metadata().transformation.value_or(
            media::kNoTransformation);
    has_frame_size_changed =
        RotationAdjustedSize(new_transform->rotation, frame->natural_size()) !=
        RotationAdjustedSize(current_video_transform.rotation,
                             current_frame_->natural_size());

    if (current_video_transform == *new_transform)
      new_transform.reset();

    if (*new_opacity == media::IsOpaque(current_frame_->format()))
      new_opacity.reset();
  }

  current_frame_ = std::move(frame);
  current_frame_is_copy_ = is_copy;

  // TODO(https://crbug.com/1050755): Improve the accuracy of these fields when
  // we only use RenderWithoutAlgorithm.
  base::TimeTicks now = base::TimeTicks::Now();
  last_presentation_time_ = now;
  last_expected_display_time_ = expected_display_time.value_or(now);
  last_preferred_render_interval_ = GetPreferredRenderInterval();
  ++presented_frames_;

  // OnNewFramePresentedCB presented_frame_cb;
  // {
  //   base::AutoLock lock(new_frame_presented_cb_lock_);
  //   presented_frame_cb = std::move(new_frame_presented_cb_);
  // }

  // if (presented_frame_cb) {
  //   std::move(presented_frame_cb).Run();
  // }

  // Complete the checks after |current_frame_| is accessible to avoid
  // deadlocks, see https://crbug.com/901744.
  PostCrossThreadTask(
      *video_frame_compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(&WhistCompositor::CheckForFrameChanges,
                          WrapRefCounted(this), is_first_frame,
                          has_frame_size_changed, std::move(new_transform),
                          std::move(new_opacity)));
}

void WhistCompositor::CheckForFrameChanges(
    bool is_first_frame,
    bool has_frame_size_changed,
    absl::optional<media::VideoTransformation> new_frame_transform,
    absl::optional<bool> new_frame_opacity) {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());

  if (is_first_frame) {
    PostCrossThreadTask(
        *main_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistPlayer::OnFirstFrameReceived, player_,
                            *new_frame_transform, *new_frame_opacity));
    return;
  }

  if (new_frame_transform.has_value()) {
    PostCrossThreadTask(
        *main_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistPlayer::OnTransformChanged, player_,
                            *new_frame_transform));
    if (submitter_)
      submitter_->SetTransform(*new_frame_transform);
  }
  if (new_frame_opacity.has_value()) {
    PostCrossThreadTask(*main_task_runner_, FROM_HERE,
                        CrossThreadBindOnce(&WhistPlayer::OnOpacityChanged,
                                            player_, *new_frame_opacity));
  }
  if (has_frame_size_changed) {
    PostCrossThreadTask(
        *main_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistPlayer::TriggerResize, player_));
    PostCrossThreadTask(
        *main_task_runner_, FROM_HERE,
        CrossThreadBindOnce(&WhistPlayer::ResetCanvasCache, player_));
  }
}

void WhistCompositor::StartRenderingInternal() {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  stopped_ = false;

  // if (video_frame_provider_client_)
  //   video_frame_provider_client_->StartRendering();
}

void WhistCompositor::StopRenderingInternal() {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  stopped_ = true;

  // if (video_frame_provider_client_)
  //   video_frame_provider_client_->StopRendering();
}

void WhistCompositor::StopUsingProviderInternal() {
  DCHECK(video_frame_compositor_task_runner_->BelongsToCurrentThread());
  // if (video_frame_provider_client_)
  //   video_frame_provider_client_->StopUsingProvider();
  // video_frame_provider_client_ = nullptr;
}

void WhistCompositor::SetAlgorithmEnabledForTesting(
    bool algorithm_enabled) {

}

void WhistCompositor::SetOnFramePresentedCallback(
    OnNewFramePresentedCB presented_cb) {
  base::AutoLock lock(new_frame_presented_cb_lock_);
  new_frame_presented_cb_ = std::move(presented_cb);
}

std::unique_ptr<WebMediaPlayer::VideoFramePresentationMetadata>
WhistCompositor::GetLastPresentedFrameMetadata() {
  auto frame_metadata =
      std::make_unique<WebMediaPlayer::VideoFramePresentationMetadata>();

  scoped_refptr<media::VideoFrame> last_frame;
  {
    base::AutoLock lock(current_frame_lock_);
    last_frame = current_frame_;
    frame_metadata->presentation_time = last_presentation_time_;
    frame_metadata->expected_display_time = last_expected_display_time_;
    frame_metadata->presented_frames = static_cast<uint32_t>(presented_frames_);

    frame_metadata->average_frame_duration = last_preferred_render_interval_;
    frame_metadata->rendering_interval = last_render_length_;
  }

  if (base::FeatureList::IsEnabled(media::kKeepRvfcFrameAlive))
    frame_metadata->frame = last_frame;

  frame_metadata->width = last_frame->visible_rect().width();
  frame_metadata->height = last_frame->visible_rect().height();

  frame_metadata->media_time = last_frame->timestamp();

  frame_metadata->metadata.MergeMetadataFrom(last_frame->metadata());

  return frame_metadata;
}

}  // namespace blink
