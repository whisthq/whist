import { values } from "lodash"
import { merge } from "rxjs"
import { pluck, startWith, withLatestFrom } from "rxjs/operators"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import TRIGGER from "@app/main/triggers/constants"

// // Gets the email from either persisted state or a successful auth
// const email = merge(
//   fromTrigger("persisted"),
//   fromTrigger("loginFlowSuccess"),
//   fromTrigger("signupFlowSuccess")
// ).pipe(pluck("email"), startWith(""))

// // Iterates through all the triggers and logs them
// values(TRIGGER).forEach((name: string) => {
//   fromTrigger(name)
//     .pipe(withLatestFrom(email))
//     .subscribe(([x, email]: [object, string]) => {
//       logBase(name, x, LogLevel.DEBUG, email).catch((err) => console.error(err))
//     })
// })
