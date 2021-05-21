/**
 * Copyright Fractal Computers, Inc. 2021
 * @file tray.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */
import { fromEvent } from "rxjs"

import { trayEvent } from "@app/utils/tray"
import { createTrigger } from "@app/utils/flows"
import { signoutAction, quitAction } from "@app/main/triggers/constants"

createTrigger(signoutAction, fromEvent(trayEvent, "signout"))
createTrigger(quitAction, fromEvent(trayEvent, "quit"))
