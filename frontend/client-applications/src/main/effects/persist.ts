import { merge } from "rxjs"
import toPairs from "lodash.topairs"

import { fromTrigger } from "@app/main/utils/flows"
import { persistSet } from "@app/main/utils/persist"
import { WhistTrigger } from "@app/constants/triggers"
import { AWS_REGIONS_SORTED_BY_PROXIMITY } from "@app/constants/store"

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

fromTrigger(WhistTrigger.awsPingRefresh).subscribe((regions) => {
  if (regions?.length > 0) persistSet(AWS_REGIONS_SORTED_BY_PROXIMITY, regions)
})
