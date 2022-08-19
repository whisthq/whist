// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_MEDIASTREAM_WHIST_PLAYER_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_MEDIASTREAM_WHIST_PLAYER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "media/renderers/paint_canvas_video_renderer.h"
#include "media/video/gpu_video_accelerator_factories.h"
#include "third_party/blink/public/common/media/display_type.h"
#include "third_party/blink/public/common/media/watch_time_reporter.h"
#include "third_party/blink/public/platform/media/webmediaplayer_delegate.h"
#include "third_party/blink/public/platform/modules/mediastream/web_media_stream.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_surface_layer_bridge.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"

namespace media {
class GpuMemoryBufferVideoFramePool;
class MediaLog;
}  // namespace media

namespace cc {
class VideoLayer;
}

namespace blink {
using CreateSurfaceLayerBridgeCB =
    base::OnceCallback<std::unique_ptr<WebSurfaceLayerBridge>(
        WebSurfaceLayerBridgeObserver*,
        cc::UpdateSubmissionStateCB)>;

class MediaStreamInternalFrameWrapper;
template <typename TimerFiredClass>
class TaskRunnerTimer;
class TimerBase;
class WebLocalFrame;
class WebMediaPlayerClient;
class WebMediaStreamAudioRenderer;
class WhistCompositor;
class MediaStreamRendererFactory;
class WebMediaStreamVideoRenderer;
class WebString;
class WebVideoFrameSubmitter;

// WhistPlayer delegates calls from WebCore::MediaPlayerPrivate to
// Chrome's media player when "src" is from media stream.
//
// All calls to WhistPlayer methods must be from the main thread of
// Renderer process.
//
// WhistPlayer works with multiple objects, the most important ones are:
//
// WebMediaStreamVideoRenderer
//   provides video frames for rendering.
//
// WebMediaPlayerClient
//   WebKit client of this media player object.
class BLINK_MODULES_EXPORT WhistPlayer
    : public WebMediaPlayer,
      public WebSurfaceLayerBridgeObserver {
 public:
  // Construct a WhistPlayer with reference to the client, and
  // a MediaStreamClient which provides WebMediaStreamVideoRenderer.
  // |delegate| must not be null.
  WhistPlayer(
      WebLocalFrame* frame,
      WebMediaPlayerClient* client,
      std::unique_ptr<media::MediaLog> media_log,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner,
      media::GpuVideoAcceleratorFactories* gpu_factories,
      CreateSurfaceLayerBridgeCB create_bridge_callback,
      std::unique_ptr<WebVideoFrameSubmitter> submitter_,
      WebMediaPlayer::SurfaceLayerMode surface_layer_mode,
      BrowserInterfaceBrokerProxy* broker);

  WhistPlayer(const WhistPlayer&) = delete;
  WhistPlayer& operator=(const WhistPlayer&) = delete;

  ~WhistPlayer() override;

  // WebMediaPlayer::LoadTiming Load() override;
  WebMediaPlayer::LoadTiming Load(LoadType,
                          const WebMediaPlayerSource&,
                          CorsMode,
                          bool is_cache_disabled) override;

  // WebSurfaceLayerBridgeObserver implementation.
  void OnWebLayerUpdated() override;
  void RegisterContentsLayer(cc::Layer* layer) override;
  void UnregisterContentsLayer(cc::Layer* layer) override;
  void OnSurfaceIdUpdated(viz::SurfaceId surface_id) override;

  // Playback controls.
  void Play() override;
  void Pause() override;
  void Seek(double seconds) override;
  void SetRate(double rate) override;
  void SetVolume(double volume) override;
  void SetLatencyHint(double seconds) override;
  void SetPreservesPitch(bool preserves_pitch) override;
  void SetWasPlayedWithUserActivation(bool was_played_with_user_activation) override;
  void OnRequestPictureInPicture() override;
  bool SetSinkId(const WebString& sink_id, WebSetSinkIdCompleteCallback completion_callback) override;
  void SetPreload(WebMediaPlayer::Preload preload) override;
  WebTimeRanges Buffered() const override;
  WebTimeRanges Seekable() const override;

  // Methods for painting.
  void Paint(cc::PaintCanvas* canvas,
             const gfx::Rect& rect,
             cc::PaintFlags& flags) override;
  scoped_refptr<media::VideoFrame> GetCurrentFrame() override;
  media::PaintCanvasVideoRenderer* GetPaintCanvasVideoRenderer() override;
  void ResetCanvasCache();

  // Methods to trigger resize event.
  void TriggerResize();

  // True if the loaded media has a playable video/audio track.
  bool HasVideo() const override;
  bool HasAudio() const override;

  // Dimensions of the video.
  gfx::Size NaturalSize() const override;

  gfx::Size VisibleSize() const override;

  // Getters of playback state.
  bool Paused() const override;
  bool Seeking() const override;
  double Duration() const override;
  double CurrentTime() const override;
  bool IsEnded() const override;

  // Internal states of loading and network.
  WebMediaPlayer::NetworkState GetNetworkState() const override;
  WebMediaPlayer::ReadyState GetReadyState() const override;

  WebMediaPlayer::SurfaceLayerMode GetVideoSurfaceLayerMode() const override;

  WebString GetErrorMessage() const override;
  bool DidLoadingProgress() override;

  bool WouldTaintOrigin() const override;

  double MediaTimeForTimeValue(double timeValue) const override;

  unsigned DecodedFrameCount() const override;
  unsigned DroppedFrameCount() const override;
  uint64_t AudioDecodedByteCount() const override;
  uint64_t VideoDecodedByteCount() const override;

  bool HasAvailableVideoFrame() const override;

  void SetVolumeMultiplier(double multiplier) override;
  void SuspendForFrameClosed() override;

  // TODO: WebMediaPlayerDelegate::Observer might be useful for when the frame is hidden and reshown

  void OnFirstFrameReceived(media::VideoTransformation video_transform,
                            bool is_opaque);
  void OnOpacityChanged(bool is_opaque);
  void OnTransformChanged(media::VideoTransformation video_transform);

  base::WeakPtr<WebMediaPlayer> AsWeakPtr() override;

  void OnDisplayTypeChanged(DisplayType) override;

  std::unique_ptr<WebMediaPlayer::VideoFramePresentationMetadata>
  GetVideoFramePresentationMetadata() override;

  void RegisterFrameSinkHierarchy() override;
  void UnregisterFrameSinkHierarchy() override;

 private:
  friend class WhistPlayerTest;

#if defined(OS_WIN)
  static const gfx::Size kUseGpuMemoryBufferVideoFramesMinResolution;
#endif  // defined(OS_WIN)

  bool IsInPictureInPicture() const;

  // Switch to SurfaceLayer, either initially or from VideoLayer.
  void ActivateSurfaceLayerForVideo();

  // Need repaint due to state change.
  void RepaintInternal();

  // Helpers that set the network/ready state and notifies the client if
  // they've changed.
  void SetNetworkState(WebMediaPlayer::NetworkState state);
  void SetReadyState(WebMediaPlayer::ReadyState state);

  // Getter method to |client_|.
  WebMediaPlayerClient* get_client() { return client_; }

  // To be run when tracks are added or removed.
  void Reload();
  void ReloadVideo();
  void ReloadAudio();

  // Helper method used for testing.
  void SetGpuMemoryBufferVideoForTesting(
      media::GpuMemoryBufferVideoFramePool* gpu_memory_buffer_pool);
  void SetMediaStreamRendererFactoryForTesting(
      std::unique_ptr<MediaStreamRendererFactory>);

  // Callback used to detect and propagate a render error.
  void OnAudioRenderErrorCallback();

  void SendLogMessage(const WTF::String& message) const;

  void StopForceBeginFrames(TimerBase*);

  WebMediaPlayer::NetworkState network_state_;
  WebMediaPlayer::ReadyState ready_state_;

  WebMediaPlayerClient* const client_;

  // Inner class used for transfering frames on compositor thread to
  // |compositor_|.
  class FrameDeliverer;
  std::unique_ptr<FrameDeliverer> frame_deliverer_;

  scoped_refptr<cc::VideoLayer> video_layer_;

  media::PaintCanvasVideoRenderer video_renderer_;

  bool paused_;
  media::VideoTransformation video_transformation_;

  std::unique_ptr<media::MediaLog> media_log_;

  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  media::GpuVideoAcceleratorFactories* gpu_factories_;

  // Used for DCHECKs to ensure methods calls executed in the correct thread.
  THREAD_CHECKER(thread_checker_);

  scoped_refptr<WhistCompositor> compositor_;

  // const WebString initial_audio_output_device_id_;

  // The last volume received by setVolume() and the last volume multiplier from
  // SetVolumeMultiplier().  The multiplier is typical 1.0, but may be less
  // if the WebMediaPlayerDelegate has requested a volume reduction
  // (ducking) for a transient sound.  Playout volume is derived by volume *
  // multiplier.
  double volume_;
  double volume_multiplier_;

  // True if playback should be started upon the next call to OnShown(). Only
  // used on Android.
  bool should_play_upon_shown_;
  WebMediaStream web_stream_;

  // IDs of the tracks currently played.
  WebString current_video_track_id_;
  WebString current_audio_track_id_;

  CreateSurfaceLayerBridgeCB create_bridge_callback_;

  std::unique_ptr<WebVideoFrameSubmitter> submitter_;

  // Whether the use of a surface layer instead of a video layer is enabled.
  WebMediaPlayer::SurfaceLayerMode surface_layer_mode_ =
      WebMediaPlayer::SurfaceLayerMode::kNever;

  // Owns the weblayer and obtains/maintains SurfaceIds for
  // kUseSurfaceLayerForVideo feature.
  std::unique_ptr<WebSurfaceLayerBridge> bridge_;

  // Whether the video is known to be opaque or not.
  bool opaque_ = true;

  bool has_first_frame_ = false;
  base::WeakPtr<WhistPlayer> weak_this_;
  base::WeakPtrFactory<WhistPlayer> weak_factory_{this};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_MODULES_MEDIASTREAM_WEBMEDIAPLAYER_MS_H_
