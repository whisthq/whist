// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2022 Whist Technologies, Inc. All rights reserved.

#include "cloud_downloader_host.h"

#include "components/download/public/common/download_create_info.h"
#include "components/download/public/common/download_url_parameters.h"
#include "content/browser/download/download_manager_impl.h"
#include "content/public/browser/browser_context.h"

#include <cwchar>

namespace content {

CloudDownloaderHost::CloudDownloaderHost(
    mojo::PendingReceiver<blink::mojom::CloudDownloaderHost> receiver,
    BrowserContext* browser_context)
    : receiver_(this, std::move(receiver)), browser_context_(browser_context) {}

CloudDownloaderHost::~CloudDownloaderHost() {}

void CloudDownloaderHost::DownloadStart(const std::string& file_path,
                                        int64_t file_size,
                                        DownloadStartCallback callback) {
  auto download_create_info = std::make_unique<download::DownloadCreateInfo>();
  download_create_info->cloud_download = true;
  // TODO : Get the real URL from Whist Server
  download_create_info->url_chain.push_back(
      GURL("https://whist_cloud_download"));
  download_create_info->has_user_gesture = true;
  download_create_info->total_bytes = file_size;
#ifdef _WIN32
  std::wstring w_file_path;
  for (char c : file_path) {
    w_file_path.push_back(std::btowc(c));
  }
  download_create_info->save_info->file_path = base::FilePath(w_file_path);
#else
  download_create_info->save_info->file_path = base::FilePath(file_path);
#endif
  DownloadManagerImpl* download_manager =
      (DownloadManagerImpl*)browser_context_->GetDownloadManager();
  DLOG(INFO) << "Calling StartDownloadItem path:" << file_path;
  callback_ = std::move(callback);
  download_manager->StartDownloadItem(
      std::move(download_create_info),
      download::DownloadUrlParameters::OnStartedCallback(),
      base::BindOnce(&CloudDownloaderHost::StartDownloadItemCallback,
                     base::Unretained(this)));
}

void CloudDownloaderHost::DownloadUpdate(int64_t opaque,
                                         int64_t bytes_so_far,
                                         int64_t bytes_per_sec) {
  download::DownloadItemImpl* impl = (download::DownloadItemImpl*)opaque;
  const std::vector<download::DownloadItem::ReceivedSlice> received_slices;
  impl->DestinationUpdate(bytes_so_far, bytes_per_sec, received_slices);
}

void CloudDownloaderHost::DownloadComplete(int64_t opaque) {
  download::DownloadItemImpl* impl = (download::DownloadItemImpl*)opaque;
  std::unique_ptr<crypto::SecureHash> hash_state(
      crypto::SecureHash::Create(crypto::SecureHash::SHA256));
  impl->DestinationCompleted(impl->GetTotalBytes(), std::move(hash_state));
  // TODO : Need to check where impl is freed. Does Download Manager free it
  // internally? If not it has to be freed here.
}

void CloudDownloaderHost::StartDownloadItemCallback(
    std::unique_ptr<download::DownloadCreateInfo> info,
    download::DownloadItemImpl* impl,
    bool should_persist) {
  std::move(callback_).Run((int64_t)impl);
}

}  // namespace content
