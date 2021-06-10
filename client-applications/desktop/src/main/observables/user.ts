import { merge } from "rxjs"
import { pluck, startWith } from "rxjs/operators"

import { fromTrigger } from "@app/utils/flows"

export const email = merge(
  fromTrigger("persisted"),
  fromTrigger("authFlowSuccess")
).pipe(pluck("email"), startWith(undefined))
