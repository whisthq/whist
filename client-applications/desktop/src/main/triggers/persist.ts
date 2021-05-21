/**
 * Copyright Fractal Computers, Inc. 2021
 * @file persist.ts
 * @brief This file contains RXJS observables that deal with state persistence (i.e. shared state).
 */

import { fromEvent } from "rxjs"
import { persisted } from "@app/utils/persist"

import { createTrigger } from "@app/utils/flows"

createTrigger("persisted", fromEvent(persisted, "data-persisted"))

createTrigger("notPersisted", fromEvent(persisted, "data-not-persisted"))
