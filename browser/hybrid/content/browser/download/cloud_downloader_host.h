// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#ifndef CONTENT_BROWSER_DOWNLOAD_CLOUD_DOWNLOADER_HOST_H_
#define CONTENT_BROWSER_DOWNLOAD_CLOUD_DOWNLOADER_HOST_H_

#include "base/callback.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "third_party/blink/public/mojom/cloud_tab/download.mojom.h"
#include "content/common/content_export.h"
#include "content/public/browser/global_routing_id.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "url/origin.h"

namespace download {
    struct DownloadCreateInfo;
    class DownloadItemImpl;
}

namespace content {

class BrowserContext;

// This class is not thread-safe, so each instance must be used on one sequence.
class CONTENT_EXPORT CloudDownloaderHost
    : public blink::mojom::CloudDownloaderHost {
 public:
  CloudDownloaderHost(
      mojo::PendingReceiver<blink::mojom::CloudDownloaderHost> receiver,
      BrowserContext* browser_context);
  ~CloudDownloaderHost() override;
  void DownloadStart(const std::string& filepath,
                     int64_t file_size,
                     DownloadStartCallback callback) override;
  void DownloadUpdate(int64_t opaque, int64_t bytes_so_far, int64_t bytes_per_sec) override;
  void DownloadComplete(int64_t opaque) override;
  void StartDownloadItemCallback(
      std::unique_ptr<download::DownloadCreateInfo> info,
      download::DownloadItemImpl* impl,
      bool should_persist);

 private:
  mojo::Receiver<blink::mojom::CloudDownloaderHost> receiver_;
  BrowserContext* browser_context_;
  DownloadStartCallback callback_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_CLOUD_DOWNLOADER_HOST_H_
