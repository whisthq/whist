// This file contains all the observables that manage the stripe payment logic
// in the renderer threadId. They listen to IPC events and communicate with
// the webserver to gain the necessary stripe info to relay back to the renderer thread.

import { from, merge } from "rxjs"
import { debugObservables, errorObservables } from "@app/utils/logging"
import {
  stripeCheckoutCreate,
  stripeCheckoutValid,
  stripeCheckoutError,
  stripePortalCreate,
  stripePortalValid,
  stripePortalError,
} from "@app/utils/payment"
import { filter, map, share, exhaustMap, tap } from "rxjs/operators"
import {
  stripeCheckoutAction,
  stripePortalAction,
} from "@app/main/events/actions"

// Listens for a checkout portal request from the main IPC channel
export const stripeCheckoutRequest = stripeCheckoutAction.pipe(
  tap((req) => console.log(req)),
  filter(
    (req) => (req?.customerId ?? "") !== "" && (req?.priceId ?? "") !== ""
  ),
  map(({ customerId, priceId, successUrl, cancelUrl }) => [
    customerId,
    priceId,
    successUrl,
    cancelUrl,
  ]),
  share()
)

// Waits for a webserver call to get the stripe checkout id to send
// back to the renderer thread
export const stripeCheckoutProcess = stripeCheckoutRequest.pipe(
  map(
    async ([customerId, priceId, successUrl, cancelUrl]) =>
      await stripeCheckoutCreate(customerId, priceId, successUrl, cancelUrl)
  ),
  exhaustMap((req) => from(req)),
  share()
)

// Emits upon webserver call checkout portal success
export const stripeCheckoutSuccess = stripeCheckoutProcess.pipe(
  filter((res) => stripeCheckoutValid(res))
)

// Emits upon webserver call checkout portal failure
export const stripeCheckoutFailure = stripeCheckoutProcess.pipe(
  filter((res) => stripeCheckoutError(res))
)

// Listens for a customer portal request from the main IPC channel
export const stripePortalRequest = stripePortalAction.pipe(
  filter(
    (req) => (req?.customerId ?? "") !== "" && (req?.returnUrl ?? "") !== ""
  ),
  map(({ customerId, returnUrl }) => [customerId, returnUrl]),
  share()
)

// Waits for a webserver call to get the customer portal URL to send
// back to the renderer thread.
export const stripePortalProcess = stripePortalRequest.pipe(
  map(
    async ([customerId, returnUrl]) =>
      await stripePortalCreate(customerId, returnUrl)
  ),
  exhaustMap((req) => from(req)),
  share()
)

// Emits upon webserver call customer portal success
export const stripePortalSuccess = stripePortalProcess.pipe(
  filter((res) => stripePortalValid(res))
)

// Emits upon webserver call customer portal failure
export const stripePortalFailure = stripePortalProcess.pipe(
  filter((res) => stripePortalError(res))
)

// Combines both checkout and customer portal action into a single observable
// to send to the renderer thread using merge(), which allows the renderer
// thread to parse the returned object accordingly.
export const stripeAction = merge(
  stripeCheckoutSuccess.pipe(
    map((req) => ({ action: "CHECKOUT", stripeCheckoutId: req.json.sessionId }))
  ),
  stripePortalSuccess.pipe(
    map((req) => ({ action: "PORTAL", stripePortalUrl: req.json.url }))
  )
)

debugObservables(
  [stripeCheckoutRequest, "stripeCheckoutRequest"],
  [stripeCheckoutProcess, "stripeCheckoutProcess"],
  [stripeCheckoutSuccess, "stripeCheckoutSuccess"],
  [stripePortalRequest, "stripePortalRequest"],
  [stripePortalProcess, "stripePortalProcess"],
  [stripePortalSuccess, "stripePortalSuccess"],
  [stripeAction, "stripeAction"]
)

errorObservables(
  [stripeCheckoutFailure, "stripeCheckoutFailure"],
  [stripePortalFailure, "stripePortalFailure"]
)
