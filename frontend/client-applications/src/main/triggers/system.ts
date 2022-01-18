/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file power.ts
 * @brief Turns events emitted by powerMonitor into observables, used to monitor the computer's state.
 */

import { powerMonitor } from "electron"
import { fromEvent } from "rxjs"

import { createTrigger } from "@app/main/utils/flows"
import { WhistTrigger } from "@app/constants/triggers"
import { networkAnalysisEvent } from "@app/main/utils/networkAnalysis"

// Fires when your computer wakes up
createTrigger(WhistTrigger.powerResume, fromEvent(powerMonitor, "resume"))
// Fires when your computer goes to sleep
createTrigger(WhistTrigger.powerSuspend, fromEvent(powerMonitor, "suspend"))
// Fires when network analysis test is done running
createTrigger(
  WhistTrigger.networkAnalysisEvent,
  fromEvent(networkAnalysisEvent, "did-update")
)
