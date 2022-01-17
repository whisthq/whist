import { fromEvent } from "rxjs"

import { createTrigger } from "@app/main/utils/flows"
import { stripeEvent } from "@app/main/utils/windows"
import { WhistTrigger } from "@app/constants/triggers"

createTrigger(
  WhistTrigger.stripeAuthRefresh,
  fromEvent(stripeEvent, "stripe-auth-refresh")
)
createTrigger(
  WhistTrigger.stripePaymentError,
  fromEvent(stripeEvent, "stripe-payment-error")
)
