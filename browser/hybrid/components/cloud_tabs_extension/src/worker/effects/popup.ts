import uniq from "lodash.uniq"
import { merge, of } from "rxjs"
import { withLatestFrom, takeUntil } from "rxjs/operators"

import {
  authSuccess,
  authFailure,
  authNetworkError,
} from "@app/worker/events/auth"
import { tabActivated, tabFocused, tabUpdated } from "@app/worker/events/tabs"

import { stateDidChange, whistState } from "@app/worker/utils/state"
import { getStorage, setStorage } from "@app/worker/utils/storage"
import {
  convertToCloudTab,
  getActiveTab,
  getNumberOfCloudTabs,
  isCloudTab,
  removeTabFromQueue,
  stripCloudUrl,
  updateTabUrl,
} from "@app/worker/utils/tabs"
import { cannotStreamError } from "@app/worker/utils/errors"
import {
  popupActivateCloudTab,
  popupClicked,
  popupDeactivateCloudTab,
  popupOpened,
  popupUrlSaved,
  popupUrlUnsaved,
  popupOpenLogin,
  popupOpenIntercom,
  popupInviteCode,
} from "@app/worker/events/popup"
import { Storage } from "@app/constants/storage"
import { inviteCode } from "@app/constants/app"
import { AWSRegion, regions } from "@app/constants/location"

// If the popup is opened, send the necessary display information
popupOpened
  .pipe(withLatestFrom(authSuccess))
  .subscribe(async ([event]: [any]) => {
    void Promise.all([
      getNumberOfCloudTabs(),
      getActiveTab(),
      getStorage<boolean | undefined>(Storage.WAITLISTED),
      getStorage<string[]>(Storage.SAVED_CLOUD_URLS),
      getStorage<Array<{ region: AWSRegion; pingTime: number }>>(
        Storage.CLOSEST_AWS_REGIONS
      ),
    ]).then(
      ([
        numberCloudTabs,
        activeTab,
        waitlisted,
        savedCloudUrls,
        sortedRegions,
      ]) => {
        const url = new URL(stripCloudUrl(activeTab.url ?? ""))
        const host =
          url?.protocol === "http:" || url?.protocol === "https:"
            ? url?.hostname
            : undefined

        event.sendResponse({
          isCloudTab: isCloudTab(activeTab),
          isWaiting: whistState.waitingCloudTabs.some(
            (tab: chrome.tabs.Tab) => activeTab.id === tab.id
          ),
          isSaved: (savedCloudUrls ?? []).some(
            (_host: string) => _host === host
          ),
          error: cannotStreamError(activeTab),
          numberCloudTabs,
          host,
          isLoggedIn: true,
          waitlisted: waitlisted ?? true,
          networkInfo: {
            ...sortedRegions[0],
            region: regions.find((r) => r.name === sortedRegions[0]?.region)
              ?.location,
          },
        })
      }
    )
  })

popupOpened
  .pipe(withLatestFrom(authFailure), takeUntil(authSuccess))
  .subscribe(([event]: [any]) => {
    event.sendResponse({ isLoggedIn: false })
  })

popupOpened
  .pipe(withLatestFrom(authNetworkError), takeUntil(authSuccess))
  .subscribe(([event]: [any]) => {
    event.sendResponse({ networkError: true })
  })

merge(popupActivateCloudTab, popupClicked)
  .pipe(withLatestFrom(authSuccess))
  .subscribe(() => {
    void Promise.all([
      getActiveTab(),
      getStorage<boolean | undefined>(Storage.WAITLISTED),
    ]).then(([tab, waitlisted]: [chrome.tabs.Tab, boolean | undefined]) => {
      if (
        !(waitlisted ?? true) &&
        cannotStreamError(tab) === undefined &&
        !isCloudTab(tab)
      ) {
        convertToCloudTab(tab)
      }
    })
  })

// If the user disables a cloud tab
popupDeactivateCloudTab.subscribe((event: any) => {
  void Promise.all([getStorage(Storage.SAVED_CLOUD_URLS), getActiveTab()]).then(
    (args: any) => {
      const [savedCloudUrls, tab] = args
      const url = new URL(stripCloudUrl(tab.url))
      const host = url.hostname

      removeTabFromQueue(tab)

      void setStorage(
        Storage.SAVED_CLOUD_URLS,
        savedCloudUrls?.filter((h: string) => h !== host)
      ).then(() => {
        updateTabUrl(tab.id, stripCloudUrl(tab.url ?? ""))
        event.sendResponse()
      })
    }
  )
})

// If the user saves a URL to always be streamed
popupUrlSaved.subscribe((event: any) => {
  void Promise.all([getStorage(Storage.SAVED_CLOUD_URLS), getActiveTab()]).then(
    (args: any) => {
      const [savedCloudUrls, tab] = args
      const url = new URL(stripCloudUrl(tab.url))
      const host = url.hostname

      void setStorage(
        Storage.SAVED_CLOUD_URLS,
        uniq([...(savedCloudUrls ?? []), host])
      ).then(() => event.sendResponse())
    }
  )
})

// If the user unsaves a URL from always being streamed
popupUrlUnsaved.subscribe((event: any) => {
  void Promise.all([getStorage(Storage.SAVED_CLOUD_URLS), getActiveTab()]).then(
    (args: any) => {
      const [savedCloudUrls, tab] = args
      const url = new URL(stripCloudUrl(tab.url))
      const host = url.hostname

      void setStorage(
        Storage.SAVED_CLOUD_URLS,
        (savedCloudUrls ?? [])?.filter((_host: string) => _host !== host)
      ).then(() => event.sendResponse)
    }
  )
})

// If the user clicks the popup login button
popupOpenLogin.subscribe(() => {
  chrome.tabs.create({ url: "chrome://welcome" })
})

// If the user clicks the popup help button
popupOpenIntercom.subscribe(() => {
  chrome.tabs.create({ url: chrome.runtime.getURL("intercom.html") })
})

popupInviteCode.subscribe((event: any) => {
  const success = event?.request?.value?.code === inviteCode
  event.sendResponse({ success })

  if (success) void setStorage(Storage.WAITLISTED, false)
})

merge(
  tabActivated,
  tabFocused,
  tabUpdated,
  stateDidChange("waitingCloudTabs"),
  of(null)
).subscribe(async () => {
  const activeTab = await getActiveTab()
  const waitlisted =
    (await getStorage<boolean | undefined>(Storage.WAITLISTED)) ?? true

  if (isCloudTab(activeTab)) {
    chrome.browserAction.setIcon({
      path: "../icons/cloud-on.png",
    })
  } else {
    chrome.browserAction.setIcon({
      path: "../icons/cloud-off.png",
    })
  }

  if (
    waitlisted ||
    cannotStreamError(activeTab) !== undefined ||
    isCloudTab(activeTab)
  ) {
    chrome.browserAction.setPopup({ popup: "../popup.html" })
  } else {
    chrome.browserAction.setPopup({ popup: "" })
  }
})
