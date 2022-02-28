import { filter, map } from "rxjs/operators"
import {
  accessToken,
  subscriptionStatus,
  subscriptionStatusParse,
  hasValidSubscription,
} from "@whist/core-ts"

import { flow } from "@app/main/utils/flows"

export default flow<accessToken>("checkPaymentFlow", (trigger) => {
  const parsed = trigger.pipe(
    map((x: accessToken) => ({
      ...x,
      ...subscriptionStatusParse(x),
    }))
  )

  return {
    success: parsed.pipe(
      filter((tokens: subscriptionStatus) => hasValidSubscription(tokens))
    ),
    failure: parsed.pipe(
      filter((tokens: subscriptionStatus) => !hasValidSubscription(tokens))
    ),
  }
})
