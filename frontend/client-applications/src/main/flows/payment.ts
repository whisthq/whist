import { merge } from "rxjs"
import { map, filter, share } from "rxjs/operators"
import {
  accessToken,
  refreshToken,
  subscriptionStatusParse,
  hasValidSubscription,
} from "@whist/core-ts"

import { flow, fork } from "@app/main/utils/flows"
import { authRefreshFlow } from "@app/main/flows/auth"

export default flow<accessToken & refreshToken>(
  "checkPaymentFlow",
  (trigger) => {
    const success = trigger.pipe(
      map((x: accessToken & refreshToken) => ({
        ...x,
        ...subscriptionStatusParse(x),
      })),
      filter((res) => hasValidSubscription(res))
    )

    const failure = trigger.pipe(
      map((x: accessToken & refreshToken) => ({
        ...x,
        ...subscriptionStatusParse(x),
      })),
      filter((res) => !hasValidSubscription(res))
    )

    const refreshed = authRefreshFlow(failure)

    const successAfterRefresh = refreshed.success.pipe(
      map((x: accessToken & refreshToken) => ({
        ...x,
        ...subscriptionStatusParse(x),
      })),
      filter((res) => hasValidSubscription(res))
    )

    const failureAfterRefresh = refreshed.success.pipe(
      map((x: accessToken & refreshToken) => ({
        ...x,
        ...subscriptionStatusParse(x),
      })),
      filter((res) => !hasValidSubscription(res))
    )
    return {
      success: merge(success, successAfterRefresh),
      failure: merge(refreshed.failure, failureAfterRefresh),
    }
  }
)
