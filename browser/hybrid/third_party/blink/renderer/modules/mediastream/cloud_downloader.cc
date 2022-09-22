// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "cloud_downloader.h"

#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier_media.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "whist/browser/hybrid/third_party/whist/protocol_client_interface.h"

namespace blink {

static CloudDownloader* singleton_ = nullptr;

CloudDownloader* CloudDownloader::GetCloudDownloader() {
  return singleton_;
}

void CloudDownloader::CreateCloudDownloader(
    BrowserInterfaceBrokerProxy* broker) {
  if (singleton_)
    return;
  singleton_ = new CloudDownloader(broker);
  WHIST_VIRTUAL_INTERFACE_CALL(
      file.set_on_file_download_start_callback,
      [](const char* file_path, int64_t file_size) -> void* {
        PostCrossThreadTask(
            *(Thread::MainThread()->GetTaskRunner()), FROM_HERE,
            CrossThreadBindOnce(&CloudDownloader::OnDownloadStart,
                                singleton_->GetWeakPtr(),
                                std::string(file_path), file_size));
        return singleton_->GetOpaqueContext();
      });
  WHIST_VIRTUAL_INTERFACE_CALL(
      file.set_on_file_download_update_callback,
      [](void* opaque, int64_t bytes_so_far, int64_t bytes_per_sec) {
        PostCrossThreadTask(
            *(Thread::MainThread()->GetTaskRunner()), FROM_HERE,
            CrossThreadBindOnce(&CloudDownloader::OnDownloadUpdate,
                                singleton_->GetWeakPtr(), (int64_t)opaque,
                                bytes_so_far, bytes_per_sec));
      });
  WHIST_VIRTUAL_INTERFACE_CALL(
      file.set_on_file_download_complete_callback, [](void* opaque) {
        PostCrossThreadTask(
            *(Thread::MainThread()->GetTaskRunner()), FROM_HERE,
            CrossThreadBindOnce(&CloudDownloader::OnDownloadComplete,
                                singleton_->GetWeakPtr(), (int64_t)opaque));
      });
}

CloudDownloader::CloudDownloader(BrowserInterfaceBrokerProxy* broker) {
  broker->GetInterface(cloud_downloader_host_.BindNewPipeAndPassReceiver());
}

void CloudDownloader::OnDownloadStart(const std::string& file_path,
                                      int64_t file_size) {
  cloud_downloader_host_->DownloadStart(
      file_path, file_size,
      WTF::Bind(&CloudDownloader::DownloadStartCallback, GetWeakPtr()));
}

void CloudDownloader::OnDownloadUpdate(int64_t opaque,
                                       int64_t bytes_so_far,
                                       int64_t bytes_per_sec) {
  cloud_downloader_host_->DownloadUpdate(opaque, bytes_so_far, bytes_per_sec);
}

void CloudDownloader::OnDownloadComplete(int64_t opaque) {
  cloud_downloader_host_->DownloadComplete(opaque);
}

void* CloudDownloader::GetOpaqueContext() {
  waitable_event_.Wait();
  waitable_event_.Reset();
  return (void*)opaque_;
}

void CloudDownloader::DownloadStartCallback(int64_t opaque) {
  opaque_ = opaque;
  waitable_event_.Signal();
}

}  // namespace blink
