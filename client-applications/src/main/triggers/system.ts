/**
 * Copyright Fractal Computers, Inc. 2021
 * @file power.ts
 * @brief Turns events emitted by powerMonitor into observables, used to monitor the computer's state.
 */

import { powerMonitor } from "electron"
import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

// Fires when your computer wakes up
createTrigger(TRIGGER.powerResume, fromEvent(powerMonitor, "resume"))
// Fires when your computer goes to sleep
createTrigger(TRIGGER.powerSuspend, fromEvent(powerMonitor, "suspend"))
