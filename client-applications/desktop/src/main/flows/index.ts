import { merge, Observable } from "rxjs"
import { map, take } from "rxjs/operators"

import authFlow from "@app/main/flows/auth"
import mandelboxFlow from "@app/main/flows/mandelbox"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import { fromTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"
import { getRegionFromArgv } from "@app/utils/region"
import { AWSRegion } from "@app/@types/aws"

// Autoupdate flow
autoUpdateFlow(fromTrigger("updateAvailable"))

// Auth flow
authFlow(
  fromSignal(
    merge(
      fromSignal(fromTrigger("authInfo"), fromTrigger("notPersisted")),
      fromTrigger("persisted")
    ),
    fromTrigger("updateNotAvailable")
  )
)

// Observable that fires when Fractal is ready to be launched
const launchTrigger = fromTrigger("authFlowSuccess").pipe(
  map((x: object) => ({
    ...x, // { sub, accessToken, configToken }
    region: getRegionFromArgv(process.argv), // AWS region, if admins want to control the region
  })),
  take(1)
) as Observable<{
  sub: string
  accessToken: string
  configToken: string
  region?: AWSRegion
}>

// Mandelbox creation flow
mandelboxFlow(launchTrigger)
