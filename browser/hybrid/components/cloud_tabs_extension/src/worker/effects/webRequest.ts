import { Storage } from "@app/constants/storage"

import { webRequest } from "@app/worker/events/webRequest"
import { getStorage } from "@app/worker/utils/storage"
import {
  getTab,
  updateTabUrl,
  unmarkActiveCloudTab,
  stripCloudUrl,
} from "@app/worker/utils/tabs"

/* eslint-disable @typescript-eslint/strict-boolean-expressions */
webRequest.subscribe((response: any) => {
  void Promise.all([
    getStorage<string[]>(Storage.SAVED_CLOUD_URLS),
    getTab(response.tabId),
  ]).then((args: any) => {
    const [savedCloudUrls, tab] = args
    const url = new URL(response.url)
    const host = url.hostname
    const currentHost = new URL(stripCloudUrl(tab.url)).hostname
    const isSaved = (savedCloudUrls ?? []).includes(host)
    const isCloudUrl = response.url.startsWith("cloud:")

    if (!isSaved && !isCloudUrl) return

    if (!isCloudUrl && host !== currentHost) {
      unmarkActiveCloudTab(tab)
      void updateTabUrl(tab.id, `cloud:${response.url as string}`)
    }
  })
})
