// This file is home to observables related to error handling in the application.
// These should subscribe to "Failure" observables, and emit useful data
// for error-related side-effects defined in effects.ts.

import { flow } from "@app/utils/flows"
import { closeWindows } from "@app/main/flows/error/utils"

export default flow("error", (name, trigger) => {
  trigger.subscribe(({name, payload}) => {
    closeWindows()
  })
})
