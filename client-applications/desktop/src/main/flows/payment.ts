import { filter } from "rxjs/operators"
import { subscriptionStatus, hasValidSubscription } from "@fractal/core-ts"

import { flow } from "@app/utils/flows"

export default flow<{
  userEmail: string
  accessToken: string
  refreshToken: string
  configToken?: string
  subscriptionStatus: string
}>("checkPaymentFlow", (trigger) => {
  return {
    success: trigger.pipe(
      filter((tokens: subscriptionStatus) => hasValidSubscription(tokens))
    ),
    failure: trigger.pipe(
      filter((tokens: subscriptionStatus) => !hasValidSubscription(tokens))
    ),
  }
})
