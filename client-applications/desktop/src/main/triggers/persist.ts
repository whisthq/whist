/**
 * Copyright Fractal Computers, Inc. 2021
 * @file persist.ts
 * @brief This file contains RXJS observables that deal with state persistence (i.e. shared state).
 */

import { fromEvent } from "rxjs"
import { persisted as persistEvent } from "@app/utils/persist"

import { createTrigger } from "@app/utils/flows"
import TRIGGER from "@app/utils/triggers"

createTrigger(TRIGGER.persisted, fromEvent(persistEvent, "data-persisted"))
createTrigger(
  TRIGGER.notPersisted,
  fromEvent(persistEvent, "data-not-persisted")
)
