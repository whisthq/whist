import { merge } from "rxjs"
import toPairs from "lodash.topairs"

import { fromTrigger } from "@app/utils/flows"
import { persistSet } from "@app/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"

// On a successful auth, store the auth credentials in Electron store
// so the user is remembered
merge(
  fromTrigger(WhistTrigger.authFlowSuccess),
  fromTrigger(WhistTrigger.authRefreshSuccess)
).subscribe(
  (args: {
    userEmail?: string
    accessToken?: string
    refreshToken?: string
    configToken?: string
  }) => {
    toPairs(args).forEach(([key, value]) => {
      if (value !== undefined) persistSet(`auth.${key}`, value)
    })
  }
)
