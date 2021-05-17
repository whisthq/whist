/**
 * Copyright Fractal Computers, Inc. 2021
 * @file tray.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */
import { fromEvent } from "rxjs"

import { trayEvent } from "@app/utils/tray"
import { createTrigger } from "@app/utils/flows"
import { EventEmitter } from "events"

createTrigger("signout", fromEvent(trayEvent as EventEmitter, "signout"))
createTrigger("quit", fromEvent(trayEvent as EventEmitter, "quit"))
