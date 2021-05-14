/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file contains all RXJS observables created from events triggered by authentication
 */

import { Subject } from "rxjs"
import { filter, map } from "rxjs/operators"

export interface AuthEvent {
  type: AuthEventType
  payload: Record<string, any> | null
}

export enum AuthEventType {
  SIGNOUT = "signoutRequest",
  QUIT = "quitRequest",
}

export const eventAuth = new Subject<AuthEvent>()

export const authEvents = {
  loadTokens: (data: Record<string, string>) =>
    eventAuth.next({ type: AuthEventType.SIGNOUT, payload: data }),
  refreshTokens: (data: Record<string, string>) =>
    eventAuth.next({ type: AuthEventType.QUIT, payload: data }),
}

export const loadTokens = eventAuth.pipe(
  filter((x) => x !== undefined),
  map((x) => x.payload)
)

export const refreshTokens = eventAuth.pipe(
  filter((x) => x !== undefined),
  map((x) => x.payload)
)
