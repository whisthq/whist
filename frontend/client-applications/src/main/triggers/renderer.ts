import { Observable } from "rxjs"
import { filter, map, pluck } from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

const filterByName = (
  observable: Observable<{ name: string; payload: any }>,
  name: string
) => {
  return observable.pipe(
    filter(
      (x: { name: string; payload: any }) => x !== undefined && x.name === name
    ),
    map((x: { name: string; payload: any }) => x.payload)
  )
}

;[
  WhistTrigger.relaunchAction, // Fires when "Continue" button is clicked on error window popup
  WhistTrigger.clearCacheAction, // Fires when "Signout" button is clicked on signout window popup
  WhistTrigger.showAuthWindow,
  WhistTrigger.showSignoutWindow,
  WhistTrigger.showPaymentWindow,
  WhistTrigger.showSupportWindow,
  WhistTrigger.showSpeedtestWindow,
  WhistTrigger.showLicenseWindow,
  WhistTrigger.showImportWindow,
  WhistTrigger.showRestoreTabsWindow,
  WhistTrigger.setDefaultBrowser,
  WhistTrigger.importTabs,
  WhistTrigger.restoreLastSession,
  WhistTrigger.allowNonUSServers,
  WhistTrigger.getOtherBrowserWindows,
  WhistTrigger.closeSupportWindow,
  WhistTrigger.beginImport,
  WhistTrigger.emitIPC,
  WhistTrigger.startNetworkAnalysis,
  WhistTrigger.openExternalURL,
].forEach((trigger: string) => {
  createTrigger(
    trigger,
    filterByName(
      fromTrigger(WhistTrigger.eventIPC).pipe(pluck("trigger")),
      trigger
    )
  )
})
