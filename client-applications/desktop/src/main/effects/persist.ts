import { merge } from "rxjs"
import { toPairs } from "lodash"

import { fromTrigger } from "@app/utils/flows"
import { persist } from "@app/utils/persist"

// On a successful auth, store the email and tokens in Electron store
// so the user is remembered
merge(fromTrigger("authFlowSuccess")).subscribe(
  (args: {
    email: string
    sub: string
    accessToken: string
    refreshToken: string
    configToken: string
  }) => {
    toPairs(args).forEach(([key, value]) => {
      persist(key, value)
    })
  }
)
