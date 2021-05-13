/**
 * Copyright Fractal Computers, Inc. 2021
 * @file tray.ts
 * @brief This file contains all RXJS observables created from events caused by clicking on the tray.
 */
import { fromEvent } from "rxjs"

import { trayEvent } from "@app/main/utils/tray"
import { trigger } from "@app/main/utils/flows"
import { EventEmitter } from "events"

trigger("signout", fromEvent(trayEvent as EventEmitter, "signout"))
trigger("quit", fromEvent(trayEvent as EventEmitter, "quit"))