import { values } from "lodash"
import { withLatestFrom } from "rxjs/operators"

import { logBase, LogLevel } from "@app/utils/logging"
import { fromTrigger } from "@app/utils/flows"
import { email } from "@app/main/observables/user"
import TRIGGER from "@app/utils/triggers"

// Iterates through all the triggers and logs them
values(TRIGGER).forEach((name: string) => {
  fromTrigger(name)
    .pipe(withLatestFrom(email))
    .subscribe(([x, email]: [object, string]) => {
      logBase(name, x, LogLevel.DEBUG, email).catch((err) => console.error(err))
    })
})
