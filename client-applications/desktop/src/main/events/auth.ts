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
  LOAD = "loadTokens",
  REFRESH = "refreshTokens",
}

export const eventAuth = new Subject<AuthEvent>()

export const authEvents = {
  loadTokens: (data: Record<string, string>) =>
    eventAuth.next({ type: AuthEventType.LOAD, payload: data }),
  refreshTokens: (data: Record<string, string>) =>
    eventAuth.next({ type: AuthEventType.REFRESH, payload: data }),
}

// Observable for when tokens are loaded for the first time
export const loadTokens = eventAuth.pipe(
  filter((x) => x !== undefined),
  map((x) => x.payload)
)

// Observable for when tokens are regenerated using a refresh token
export const refreshTokens = eventAuth.pipe(
  filter((x) => x !== undefined),
  map((x) => x.payload)
)
