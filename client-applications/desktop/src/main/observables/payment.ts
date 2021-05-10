import { fromEventIPC } from "@app/main/events/ipc"
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

export const stripeCheckoutProcess = stripeCheckoutRequest.pipe(
  map(
    async ([customerId, priceId, successUrl, cancelUrl]) =>
      await stripeCheckoutCreate(customerId, priceId, successUrl, cancelUrl)
  ),
  exhaustMap((req) => from(req)),
  share()
)

export const stripeCheckoutSuccess = stripeCheckoutProcess.pipe(
  filter((res) => stripeCheckoutValid(res))
)

export const stripeCheckoutFailure = stripeCheckoutProcess.pipe(
  filter((res) => stripeCheckoutError(res))
)

export const stripePortalRequest = stripePortalAction.pipe(
  filter(
    (req) => (req?.customerId ?? "") !== "" && (req?.returnUrl ?? "") !== ""
  ),
  map(({ customerId, returnUrl }) => [customerId, returnUrl]),
  share()
)

export const stripePortalProcess = stripePortalRequest.pipe(
  map(
    async ([customerId, returnUrl]) =>
      await stripePortalCreate(customerId, returnUrl)
  ),
  exhaustMap((req) => from(req)),
  share()
)

export const stripePortalSuccess = stripePortalProcess.pipe(
  filter((res) => stripePortalValid(res))
)

export const stripePortalFailure = stripePortalProcess.pipe(
  filter((res) => stripePortalError(res))
)

export const stripeAction = merge(
  stripeCheckoutSuccess.pipe(
    map((req) => ({ action: "CHECKOUT", stripeCheckoutId: req.json.sessionId }))
  ),
  stripePortalSuccess.pipe(
    map((req) => ({ action: "PORTAL", stripePortalUrl: req.json.url }))
  )
)

// IDEA: merge the two successes together

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
