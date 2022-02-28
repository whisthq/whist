import { filter, map, tap } from "rxjs/operators"
import {
  accessToken,
  subscriptionStatus,
  subscriptionStatusParse,
  hasValidSubscription,
} from "@whist/core-ts"

import { flow } from "@app/main/utils/flows"

export default flow<accessToken>("checkPaymentFlow", (trigger) => {
  console.log("CHECK PAYMENT STARTED")

  trigger.subscribe((x) => console.log("TRIGGER IS", x))

  const parsed = trigger.pipe(
    tap((x) => console.log("CHECKING TOKEN", x)),
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
