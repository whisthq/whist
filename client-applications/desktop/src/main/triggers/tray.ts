/**
 * Copyright Fractal Computers, Inc. 2021
 * @file tray.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */
import { fromEvent } from "rxjs"

import { trayEvent } from "@app/utils/tray"
import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

createTrigger(TRIGGER.signoutAction, fromEvent(trayEvent, "signout"))
createTrigger(TRIGGER.quitAction, fromEvent(trayEvent, "quit"))
createTrigger(TRIGGER.regionAction, fromEvent(trayEvent, "region"))
