// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "whist/browser/hybrid/third_party/blink/public/web/modules/mediastream/whist_player.h"

#include <stddef.h>

#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "cc/layers/video_frame_provider_client_impl.h"
#include "cc/layers/video_layer.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_content_type.h"
#include "media/base/media_log.h"
#include "media/base/video_frame.h"
#include "media/base/video_transformation.h"
#include "media/base/video_types.h"
#include "media/mojo/mojom/media_metrics_provider.mojom.h"
#include "media/video/gpu_memory_buffer_video_frame_pool.h"
#include "services/viz/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/mojom/mediastream/media_stream.mojom-blink.h"
#include "third_party/blink/public/platform/modules/mediastream/web_media_stream_audio_renderer.h"
#include "third_party/blink/public/platform/modules/mediastream/web_media_stream_video_renderer.h"
#include "third_party/blink/public/platform/modules/webrtc/webrtc_logging.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_media_player_source.h"
#include "third_party/blink/public/platform/web_surface_layer_bridge.h"
#include "third_party/blink/public/web/modules/media/webmediaplayer_util.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_local_frame_wrapper.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_renderer_factory.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_audio_track.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_component.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_descriptor.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_source.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread.h"
#include "whist/browser/hybrid/third_party/blink/renderer/modules/mediastream/cloud_downloader.h"
#include "whist/browser/hybrid/third_party/blink/renderer/modules/mediastream/whist_compositor.h"
#include "whist/browser/hybrid/third_party/whist/whist_protocol_client.h"


#include <iostream>
#include <thread>

namespace WTF {

template <>
struct CrossThreadCopier<viz::SurfaceId>
    : public CrossThreadCopierPassThrough<viz::SurfaceId> {
  STATIC_ONLY(CrossThreadCopier);
};

}  // namespace WTF

namespace blink {

  namespace {

enum class RendererReloadAction {
  KEEP_RENDERER,
  REMOVE_RENDERER,
  NEW_RENDERER
};

const char* ReadyStateToString(WebMediaPlayer::ReadyState state) {
  switch (state) {
    case WebMediaPlayer::kReadyStateHaveNothing:
      return "HaveNothing";
    case WebMediaPlayer::kReadyStateHaveMetadata:
      return "HaveMetadata";
    case WebMediaPlayer::kReadyStateHaveCurrentData:
      return "HaveCurrentData";
    case WebMediaPlayer::kReadyStateHaveFutureData:
      return "HaveFutureData";
    case WebMediaPlayer::kReadyStateHaveEnoughData:
      return "HaveEnoughData";
  }
}

const char* NetworkStateToString(WebMediaPlayer::NetworkState state) {
  switch (state) {
    case WebMediaPlayer::kNetworkStateEmpty:
      return "Empty";
    case WebMediaPlayer::kNetworkStateIdle:
      return "Idle";
    case WebMediaPlayer::kNetworkStateLoading:
      return "Loading";
    case WebMediaPlayer::kNetworkStateLoaded:
      return "Loaded";
    case WebMediaPlayer::kNetworkStateFormatError:
      return "FormatError";
    case WebMediaPlayer::kNetworkStateNetworkError:
      return "NetworkError";
    case WebMediaPlayer::kNetworkStateDecodeError:
      return "DecodeError";
  }
}
}  // namespace

void free_frame_ref(void* frame_ref) {
  WHIST_VIRTUAL_INTERFACE_CALL(video.free_frame_ref, frame_ref);
}

class WhistPlayer::FrameDeliverer {
 public:
  using RepaintCB = WTF::CrossThreadRepeatingFunction<
      void(scoped_refptr<media::VideoFrame> frame, bool is_copy)>;
  FrameDeliverer(const base::WeakPtr<WhistPlayer>& player,
                 RepaintCB enqueue_frame_cb, int window_id)
      : whist_window_id(window_id),
        main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
        player_(player),
        enqueue_frame_cb_(std::move(enqueue_frame_cb)) {
    DETACH_FROM_THREAD(io_thread_checker_);
  }

  FrameDeliverer(const FrameDeliverer&) = delete;
  FrameDeliverer& operator=(const FrameDeliverer&) = delete;

  ~FrameDeliverer() {
    DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);
    // Clear the video frame callback
    WHIST_VIRTUAL_INTERFACE_CALL(lifecycle.destroy_window, this->whist_window_id);
  }

  void StartFrameGetter() {
    DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);

    produce_data_timestamp_ = base::Seconds(0);

    // Store lambda info
    static std::mutex lambda_info_mutex;
    static std::map<int, std::pair<FrameDeliverer*, base::SingleThreadTaskRunner*>> lambda_info;
    lambda_info_mutex.lock();
    lambda_info[this->whist_window_id].first = this;
    lambda_info[this->whist_window_id].second = base::ThreadTaskRunnerHandle::Get().get();
    lambda_info_mutex.unlock();

    // Using the same task runner that called StartFrameGetter,
    // Set the callback to take the frames, and then process them on our task runner
    WHIST_VIRTUAL_INTERFACE_CALL(video.set_video_frame_callback, this->whist_window_id, [](int whist_window_id, void* frame_ref) -> void {
      std::lock_guard<std::mutex> guard(lambda_info_mutex);
      // Pass the frame_ref into GetAndEnqueueFrame, using the taskrunner for single-threaded usage
      lambda_info[whist_window_id].second->PostTask(FROM_HERE, base::BindOnce(&FrameDeliverer::GetAndEnqueueFrame, CrossThreadUnretained(lambda_info[whist_window_id].first), frame_ref));
    });
  }

  void GetAndEnqueueFrame(void* frame_ref) {
    DCHECK_CALLED_ON_VALID_THREAD(io_thread_checker_);

    if (frame_ref) {
      uint8_t** data;
      int* stride;
      int width;
      int height;
      int visible_width;
      int visible_height;

      WHIST_VIRTUAL_INTERFACE_CALL(video.get_frame_ref_yuv_data, frame_ref, &data, &stride, &width, &height,
                                                                 &visible_width, &visible_height);

      scoped_refptr<media::VideoFrame> frame =
          media::VideoFrame::WrapExternalYuvData(
              media::PIXEL_FORMAT_I420, gfx::Size(width, height),
              gfx::Rect(visible_width, visible_height), gfx::Size(width, height),
              stride[0], stride[1], stride[2],
              const_cast<uint8_t*>(data[0]),
              const_cast<uint8_t*>(data[1]), const_cast<uint8_t*>(data[2]),
              produce_data_timestamp_);

      frame->AddDestructionObserver(base::BindOnce(free_frame_ref, frame_ref));
      enqueue_frame_cb_.Run(std::move(frame), false);
      const base::TimeDelta timestamp_diff = base::Milliseconds(16);
      produce_data_timestamp_ += timestamp_diff;
    }
  }

 private:
  int whist_window_id;
  base::TimeDelta produce_data_timestamp_;

  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  const base::WeakPtr<WhistPlayer> player_;
  RepaintCB enqueue_frame_cb_;

  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  const scoped_refptr<base::TaskRunner> worker_task_runner_;

  // Used for DCHECKs to ensure method calls are executed on the correct thread.
  THREAD_CHECKER(io_thread_checker_);

  base::WeakPtrFactory<FrameDeliverer> weak_factory_for_pool_{this};
  base::WeakPtrFactory<FrameDeliverer> weak_factory_{this};
};

WhistPlayer::WhistPlayer(
    WebLocalFrame* frame,
    WebMediaPlayerClient* client,
    std::unique_ptr<media::MediaLog> media_log,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
    media::GpuVideoAcceleratorFactories* gpu_factories,
    CreateSurfaceLayerBridgeCB create_bridge_callback,
    std::unique_ptr<WebVideoFrameSubmitter> submitter,
    WebMediaPlayer::SurfaceLayerMode surface_layer_mode,
    BrowserInterfaceBrokerProxy* broker)
    : network_state_(WebMediaPlayer::kNetworkStateEmpty),
      ready_state_(WebMediaPlayer::kReadyStateHaveNothing),
      client_(client),
      paused_(true),
      video_transformation_(media::kNoTransformation),
      media_log_(std::move(media_log)),
      io_task_runner_(std::move(io_task_runner)),
      compositor_task_runner_(std::move(compositor_task_runner)),
      gpu_factories_(gpu_factories),
      volume_(1.0),
      volume_multiplier_(1.0),
      should_play_upon_shown_(false),
      create_bridge_callback_(std::move(create_bridge_callback)),
      submitter_(std::move(submitter)),
      surface_layer_mode_(surface_layer_mode) {
  DCHECK(client);
  weak_this_ = weak_factory_.GetWeakPtr();
  SendLogMessage(String::Format(
      "%s({is_audio_element=%s})", __func__,
      client->IsAudioElement() ? "true" : "false"));

  CloudDownloader::CreateCloudDownloader(broker);
  // TODO(tmathmeyer) WebMediaPlayerImpl gets the URL from the WebLocalFrame.
  // doing that here causes a nullptr deref.
  media_log_->AddEvent<media::MediaLogEvent::kWebMediaPlayerCreated>();
}

WhistPlayer::~WhistPlayer() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  SendLogMessage(
      String::Format("%s()", __func__));

  if (frame_deliverer_)
    io_task_runner_->DeleteSoon(FROM_HERE, frame_deliverer_.release());

  // Destruct compositor resources in the proper order.
  get_client()->SetCcLayer(nullptr);
  if (video_layer_) {
    DCHECK(surface_layer_mode_ != WebMediaPlayer::SurfaceLayerMode::kAlways);
    video_layer_->StopUsingProvider();
  }

  if (compositor_)
    compositor_->StopUsingProvider();

  // TODO: trigger main to quit

  media_log_->AddEvent<media::MediaLogEvent::kWebMediaPlayerDestroyed>();
}

void WhistPlayer::OnAudioRenderErrorCallback() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (ready_state_ == WebMediaPlayer::kReadyStateHaveNothing) {
    // Any error that occurs before reaching ReadyStateHaveMetadata should
    // be considered a format error.
    SetNetworkState(WebMediaPlayer::kNetworkStateFormatError);
  } else {
    SetNetworkState(WebMediaPlayer::kNetworkStateDecodeError);
  }
}

// WebMediaPlayer::LoadTiming WhistPlayer::Load() {
WebMediaPlayer::LoadTiming WhistPlayer::Load(LoadType,
                          const WebMediaPlayerSource& source,
                          CorsMode,
                          bool is_cache_disabled) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // SendLogMessage(String::Format("%s({load_type=%s})", __func__,
  //                               LoadTypeToString(load_type)));

  // TODO(acolwell): Change this to DCHECK_EQ(load_type, LoadTypeMediaStream)
  // once Blink-side changes land.
  // DCHECK_NE(load_type, kLoadTypeMediaSource);

  // TODO: we may have to keep an implementation of Load for WhistPlayer
  //       this will be easy because `Load` is only called directly
  //       from HTMLMediaElement, so we can call the equivalent from
  //       HTMLWhistElement.
  compositor_ = base::MakeRefCounted<WhistCompositor>(
      compositor_task_runner_, io_task_runner_,
      std::move(submitter_), surface_layer_mode_, weak_this_);

  compositor_->StartRendering();

  SetNetworkState(WebMediaPlayer::kNetworkStateLoading);
  SetReadyState(WebMediaPlayer::kReadyStateHaveNothing);

  frame_deliverer_ = std::make_unique<WhistPlayer::FrameDeliverer>(
      weak_this_,
      CrossThreadBindRepeating(&WhistCompositor::EnqueueFrame,
                               compositor_),
      source.GetAsWhistSource().GetWindowId());

  PostCrossThreadTask(
      *io_task_runner_, FROM_HERE,
      CrossThreadBindOnce(&FrameDeliverer::StartFrameGetter,
                          CrossThreadUnretained(frame_deliverer_.get())));

  // client_->DidMediaMetadataChange(HasAudio(), HasVideo(),
  //                                 media::MediaContentType::OneShot);

  return WebMediaPlayer::LoadTiming::kImmediate;
}

void WhistPlayer::OnWebLayerUpdated() {}

void WhistPlayer::RegisterContentsLayer(cc::Layer* layer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(bridge_);

  // bridge_->SetContentsOpaque(opaque_);
  client_->SetCcLayer(layer);
}

void WhistPlayer::UnregisterContentsLayer(cc::Layer* layer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // |client_| will unregister its cc::Layer if given a nullptr.
  client_->SetCcLayer(nullptr);
}

void WhistPlayer::OnSurfaceIdUpdated(viz::SurfaceId surface_id) {
  // TODO(726619): Handle the behavior when Picture-in-Picture mode is
  // disabled.
  // The viz::SurfaceId may be updated when the video begins playback or when
  // the size of the video changes.
  if (client_)
    client_->OnPictureInPictureStateChange();
}

base::WeakPtr<WebMediaPlayer> WhistPlayer::AsWeakPtr() {
  return weak_this_;
}

void WhistPlayer::Play() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WhistPlayer::Pause() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WhistPlayer::Seek(double seconds) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WhistPlayer::SetRate(double rate) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WhistPlayer::SetVolume(double volume) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WhistPlayer::SetLatencyHint(double seconds) {}

void WhistPlayer::SetPreservesPitch(bool preserves_pitch) {}

void WhistPlayer::SetWasPlayedWithUserActivation(
    bool was_played_with_user_activation) {}

void WhistPlayer::OnRequestPictureInPicture() {}

bool WhistPlayer::SetSinkId(
    const WebString& sink_id,
    WebSetSinkIdCompleteCallback completion_callback) {
  return true;
}

void WhistPlayer::SetPreload(WebMediaPlayer::Preload preload) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

bool WhistPlayer::HasVideo() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return true;
}

bool WhistPlayer::HasAudio() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return false;
}

// NOTE: leaving NaturalSize and VisibleSize as is because they don't manipulate anything
gfx::Size WhistPlayer::NaturalSize() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  const gfx::Size& current_size = compositor_->GetCurrentSize();
  if (video_transformation_.rotation == media::VIDEO_ROTATION_90 ||
      video_transformation_.rotation == media::VIDEO_ROTATION_270) {
    return gfx::Size(current_size.height(), current_size.width());
  }
  return current_size;
}

gfx::Size WhistPlayer::VisibleSize() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  scoped_refptr<media::VideoFrame> video_frame = compositor_->GetCurrentFrame();
  if (!video_frame)
    return gfx::Size();

  const gfx::Rect& visible_rect = video_frame->visible_rect();
  if (video_transformation_.rotation == media::VIDEO_ROTATION_90 ||
      video_transformation_.rotation == media::VIDEO_ROTATION_270) {
    return gfx::Size(visible_rect.height(), visible_rect.width());
  }
  return visible_rect.size();
}

bool WhistPlayer::Paused() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return paused_;
}

bool WhistPlayer::Seeking() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return false;
}

double WhistPlayer::Duration() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return std::numeric_limits<double>::infinity();
}

double WhistPlayer::CurrentTime() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // const base::TimeDelta current_time = compositor_->GetCurrentTime();
  // if (current_time.ToInternalValue() != 0)
  //   return current_time.InSecondsF();
  return 0.0;
}

bool WhistPlayer::IsEnded() const {
  // MediaStreams never end.
  return false;
}

WebMediaPlayer::NetworkState WhistPlayer::GetNetworkState() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return WebMediaPlayer::NetworkState::kNetworkStateEmpty;
}

WebMediaPlayer::ReadyState WhistPlayer::GetReadyState() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return WebMediaPlayer::ReadyState::kReadyStateHaveNothing;
}

WebMediaPlayer::SurfaceLayerMode WhistPlayer::GetVideoSurfaceLayerMode()
    const {
  return WebMediaPlayer::SurfaceLayerMode::kAlways;
}

WebString WhistPlayer::GetErrorMessage() const {
  return WebString::FromUTF8(media_log_->GetErrorMessage());
}

WebTimeRanges WhistPlayer::Buffered() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return WebTimeRanges();
}

WebTimeRanges WhistPlayer::Seekable() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return WebTimeRanges();
}

bool WhistPlayer::DidLoadingProgress() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return true;
}

void WhistPlayer::Paint(cc::PaintCanvas* canvas,
                             const gfx::Rect& rect,
                             cc::PaintFlags& flags) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // const scoped_refptr<media::VideoFrame> frame = compositor_->GetCurrentFrame();

  // scoped_refptr<viz::RasterContextProvider> provider;
  // if (frame && frame->HasTextures()) {
  //   provider = Platform::Current()->SharedMainThreadContextProvider();
  //   // GPU Process crashed.
  //   if (!provider)
  //     return;
  // }
  // const gfx::RectF dest_rect(rect);
  // video_renderer_.Paint(frame, canvas, dest_rect, flags, video_transformation_,
  //                       provider.get());
}

scoped_refptr<media::VideoFrame> WhistPlayer::GetCurrentFrame() {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return compositor_->GetCurrentFrame();
}

bool WhistPlayer::WouldTaintOrigin() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return false;
}

double WhistPlayer::MediaTimeForTimeValue(double timeValue) const {
  return base::Seconds(timeValue).InSecondsF();
}

unsigned WhistPlayer::DecodedFrameCount() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return static_cast<unsigned>(compositor_->total_frame_count());
}

unsigned WhistPlayer::DroppedFrameCount() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  return static_cast<unsigned>(compositor_->dropped_frame_count());
}

uint64_t WhistPlayer::AudioDecodedByteCount() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
  return 0;
}

uint64_t WhistPlayer::VideoDecodedByteCount() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
  return 0;
}

bool WhistPlayer::HasAvailableVideoFrame() const {
  return has_first_frame_;
}


void WhistPlayer::SuspendForFrameClosed() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WhistPlayer::SetVolumeMultiplier(double multiplier) {
  // TODO(perkj, magjed): See TODO in OnPlay().
}

void WhistPlayer::ActivateSurfaceLayerForVideo() {
  // Note that we might or might not already be in VideoLayer mode.
  DCHECK(!bridge_);

  // If we're in VideoLayer mode, then get rid of the layer.
  if (video_layer_) {
    client_->SetCcLayer(nullptr);
    video_layer_ = nullptr;
  }

  bridge_ = std::move(create_bridge_callback_)
                .Run(this, compositor_->GetUpdateSubmissionStateCallback());
  bridge_->CreateSurfaceLayer();
  bridge_->SetContentsOpaque(opaque_);

  PostCrossThreadTask(
      *compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(&WhistCompositor::EnableSubmission,
                          compositor_, bridge_->GetSurfaceId(),
                          video_transformation_, IsInPictureInPicture()));

  // If the element is already in Picture-in-Picture mode, it means that it
  // was set in this mode prior to this load, with a different
  // WebMediaPlayerImpl. The new player needs to send its id, size and
  // surface id to the browser process to make sure the states are properly
  // updated.
  // TODO(872056): the surface should be activated but for some reason, it
  // does not. It is possible that this will no longer be needed after 872056
  // is fixed.
  if (client_->GetDisplayType() == DisplayType::kPictureInPicture) {
    OnSurfaceIdUpdated(bridge_->GetSurfaceId());
  }
}

void WhistPlayer::OnFirstFrameReceived(
    media::VideoTransformation video_transform,
    bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  has_first_frame_ = true;
  OnTransformChanged(video_transform);
  OnOpacityChanged(is_opaque);

  if (surface_layer_mode_ == WebMediaPlayer::SurfaceLayerMode::kAlways)
    ActivateSurfaceLayerForVideo();

  SetReadyState(WebMediaPlayer::kReadyStateHaveMetadata);
  SetReadyState(WebMediaPlayer::kReadyStateHaveEnoughData);
  TriggerResize();
  ResetCanvasCache();
}

void WhistPlayer::OnOpacityChanged(bool is_opaque) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  opaque_ = is_opaque;
  if (!bridge_) {
    // Opacity can be changed during the session without resetting
    // |video_layer_|.
    video_layer_->SetContentsOpaque(opaque_);
  } else {
    DCHECK(bridge_);
    bridge_->SetContentsOpaque(opaque_);
  }
}

void WhistPlayer::OnTransformChanged(
    media::VideoTransformation video_transform) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  video_transformation_ = video_transform;

  if (!bridge_) {
    // Keep the old |video_layer_| alive until SetCcLayer() is called with a new
    // pointer, as it may use the pointer from the last call.
    auto new_video_layer =
        cc::VideoLayer::Create(compositor_.get(), video_transformation_);
    get_client()->SetCcLayer(new_video_layer.get());
    video_layer_ = std::move(new_video_layer);
  }
}

bool WhistPlayer::IsInPictureInPicture() const {
  // DCHECK(client_);
  // return (!client_->IsInAutoPIP() &&
  //         client_->GetDisplayType() == DisplayType::kPictureInPicture);
  return false;
}

void WhistPlayer::RepaintInternal() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  get_client()->Repaint();
}

void WhistPlayer::SetNetworkState(WebMediaPlayer::NetworkState state) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  SendLogMessage(String::Format("%s => (state=%s)", __func__,
                                NetworkStateToString(network_state_)));
  network_state_ = state;
  // Always notify to ensure client has the latest value.
  get_client()->NetworkStateChanged();
}

void WhistPlayer::SetReadyState(WebMediaPlayer::ReadyState state) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  SendLogMessage(String::Format("%s => (state=%s)", __func__,
                                ReadyStateToString(ready_state_)));
  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  get_client()->ReadyStateChanged();
}

media::PaintCanvasVideoRenderer*
WhistPlayer::GetPaintCanvasVideoRenderer() {
  return &video_renderer_;
}

void WhistPlayer::ResetCanvasCache() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  video_renderer_.ResetCache();
}

void WhistPlayer::TriggerResize() {
  if (HasVideo())
    get_client()->SizeChanged();

  client_->DidPlayerSizeChange(NaturalSize());
}

void WhistPlayer::OnDisplayTypeChanged(DisplayType display_type) {
  if (!bridge_)
    return;

  PostCrossThreadTask(
      *compositor_task_runner_, FROM_HERE,
      CrossThreadBindOnce(&WhistCompositor::SetForceSubmit,
                          CrossThreadUnretained(compositor_.get()),
                          display_type == DisplayType::kPictureInPicture));
}

void WhistPlayer::SendLogMessage(const WTF::String& message) const {
  WebRtcLogMessage("WMPMS::" + message.Utf8());
}

std::unique_ptr<WebMediaPlayer::VideoFramePresentationMetadata>
WhistPlayer::GetVideoFramePresentationMetadata() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(compositor_);

  return compositor_->GetLastPresentedFrameMetadata();
}

void WhistPlayer::RegisterFrameSinkHierarchy() {
  if (bridge_)
    bridge_->RegisterFrameSinkHierarchy();
}

void WhistPlayer::UnregisterFrameSinkHierarchy() {
  if (bridge_)
    bridge_->UnregisterFrameSinkHierarchy();
}

}  // namespace blink
