import { merge } from "rxjs"
import { pluck } from "rxjs/operators"

import { fromTrigger } from "@app/utils/flows"

export const email = merge(
  fromTrigger("persisted"),
  fromTrigger("authFlowSuccess")
).pipe(pluck("email"))