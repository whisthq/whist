import { fromEvent } from "rxjs"

import { createTrigger } from "@app/utils/flows"
import { stripeEvent } from "@app/utils/windows"
import TRIGGER from "@app/utils/triggers"

createTrigger(
  TRIGGER.stripeAuthRefresh,
  fromEvent(stripeEvent, "stripe-auth-refresh")
)
createTrigger(
  TRIGGER.stripePaymentError,
  fromEvent(stripeEvent, "stripe-payment-error")
)
