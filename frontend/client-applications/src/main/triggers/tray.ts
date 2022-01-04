/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file tray.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */
import { fromEvent } from "rxjs"

import { trayEvent } from "@app/utils/tray"
import { createTrigger } from "@app/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"

createTrigger(WhistTrigger.showPaymentWindow, fromEvent(trayEvent, "payment"))
createTrigger(WhistTrigger.showSignoutWindow, fromEvent(trayEvent, "signout"))
createTrigger(WhistTrigger.trayFeedbackAction, fromEvent(trayEvent, "feedback"))
createTrigger(WhistTrigger.trayRegionAction, fromEvent(trayEvent, "region"))
createTrigger(WhistTrigger.trayBugAction, fromEvent(trayEvent, "bug"))
createTrigger(
  WhistTrigger.trayRestoreSessionAction,
  fromEvent(trayEvent, "restore-last-browser-session")
)
createTrigger(
  WhistTrigger.trayWhistIsDefaultBrowserAction,
  fromEvent(trayEvent, "whist-is-default-browser")
)
