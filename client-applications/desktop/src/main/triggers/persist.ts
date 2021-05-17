/**
 * Copyright Fractal Computers, Inc. 2021
 * @file persist.ts
 * @brief This file contains RXJS observables that deal with state persistence (i.e. shared state).
 */

import { fromEvent } from "rxjs"
import { filter } from "rxjs/operators"
import { persisted } from "@app/utils/persist"
import { EventEmitter } from "events"

import { createTrigger } from "@app/utils/flows"

createTrigger(
  "persisted",
  fromEvent(persisted as EventEmitter, "data-persisted")
)

createTrigger(
  "notPersisted",
  fromEvent(persisted as EventEmitter, "data-not-persisted"
  )
)