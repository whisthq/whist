import { values } from "lodash"
import { merge } from "rxjs"
import { pluck, startWith, withLatestFrom } from "rxjs/operators"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/main/triggers/constants"

const email = merge(
  fromTrigger("persisted"),
  fromTrigger("loginFlowSuccess"),
  fromTrigger("signupFlowSuccess")
).pipe(pluck("email"), startWith(""))

values(TRIGGER).forEach((name: string) => {
  fromTrigger(name)
    .pipe(withLatestFrom(email))
    .subscribe(([x, email]: [object, string]) => {
      logBase(name, x, LogLevel.DEBUG, email)
    })
})
