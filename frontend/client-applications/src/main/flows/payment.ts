import { merge, Observable } from "rxjs"
import { map, filter } from "rxjs/operators"
import {
  accessToken,
  refreshToken,
  subscriptionStatusParse,
  hasValidSubscription,
} from "@whist/core-ts"

import { flow } from "@app/main/utils/flows"
import { authRefreshFlow } from "@app/main/flows/auth"

const emitIfPaymentIsValid = (obs: Observable<accessToken & refreshToken>) =>
  obs.pipe(
    map((x: accessToken & refreshToken) => ({
      ...x,
      ...subscriptionStatusParse(x),
    })),
    filter((res) => hasValidSubscription(res))
  )

const emitIfPaymentIsInvalid = (obs: Observable<accessToken & refreshToken>) =>
  obs.pipe(
    map((x: accessToken & refreshToken) => ({
      ...x,
      ...subscriptionStatusParse(x),
    })),
    filter((res) => !hasValidSubscription(res))
  )

export default flow<accessToken & refreshToken>(
  "checkPaymentFlow",
  (trigger) => {
    const success = emitIfPaymentIsValid(trigger)
    const failure = emitIfPaymentIsInvalid(trigger)

    const refreshed = authRefreshFlow(failure)

    const successAfterRefresh = emitIfPaymentIsValid(refreshed.success)
    const failureAfterRefresh = emitIfPaymentIsInvalid(refreshed.success)

    return {
      success: merge(success, successAfterRefresh),
      failure: merge(refreshed.failure, failureAfterRefresh),
    }
  }
)
