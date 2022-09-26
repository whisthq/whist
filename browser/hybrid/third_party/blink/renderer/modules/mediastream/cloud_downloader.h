// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#ifndef WHIST_BROWSER_HYBRID_THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_CLOUD_DOWNLOADER_H_
#define WHIST_BROWSER_HYBRID_THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_CLOUD_DOWNLOADER_H_

#include "base/synchronization/waitable_event.h"
#include "whist/browser/hybrid/third_party/blink/public/mojom/cloud_tab/download.mojom.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"

namespace blink {

class CloudDownloader {
 public:
  static CloudDownloader* GetCloudDownloader();
  static void CreateCloudDownloader(BrowserInterfaceBrokerProxy* broker);
  CloudDownloader(const CloudDownloader&) = delete;
  CloudDownloader& operator=(const CloudDownloader&) = delete;
  auto GetWeakPtr() { return weak_ptr_factory_.GetWeakPtr(); }
  void OnDownloadStart(const std::string& file_path, int64_t file_size);
  void OnDownloadUpdate(int64_t opaque, int64_t bytes_so_far, int64_t bytes_per_sec);
  void OnDownloadComplete(int64_t opaque);
  void* GetOpaqueContext();
  void DownloadStartCallback(int64_t opaque);
 private:
  explicit CloudDownloader(BrowserInterfaceBrokerProxy* broker);
  mojo::Remote<blink::mojom::CloudDownloaderHost> cloud_downloader_host_;
  base::WaitableEvent waitable_event_;
  int64_t opaque_;
  base::WeakPtrFactory<CloudDownloader> weak_ptr_factory_{this};
};

}  // namespace blink

#endif  // WHIST_BROWSER_HYBRID_THIRD_PARTY_BLINK_RENDERER_MODULES_MEDIASTREAM_CLOUD_DOWNLOADER_H_
