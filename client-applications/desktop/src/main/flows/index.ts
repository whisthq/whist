import { merge, Observable } from "rxjs"
import { map, take } from "rxjs/operators"

import authFlow, { authRefreshFlow } from "@app/main/flows/auth"
import mandelboxFlow from "@app/main/flows/mandelbox"
import autoUpdateFlow from "@app/main/flows/autoupdate"
import { fromTrigger, createTrigger } from "@app/utils/flows"
import { fromSignal } from "@app/utils/observables"
import { getRegionFromArgv } from "@app/utils/region"
import { AWSRegion } from "@app/@types/aws"
import TRIGGER from "@app/utils/triggers"

// Autoupdate flow
const update = autoUpdateFlow(fromTrigger("updateAvailable"))

// Auth flow
const auth = authFlow(
    fromSignal(
        merge(
            fromSignal(
                fromTrigger("authInfo"),
                fromTrigger(TRIGGER.notPersisted)
            ),
            fromTrigger(TRIGGER.persisted)
        ),
        merge(
            fromTrigger(TRIGGER.updateNotAvailable),
            fromTrigger(TRIGGER.updateError)
        )
    )
)

// Observable that fires when Fractal is ready to be launched
const launchTrigger = fromTrigger(TRIGGER.authFlowSuccess).pipe(
    map((x: object) => ({
        ...x, // { subClaim, accessToken, configToken }
        region: getRegionFromArgv(process.argv), // AWS region, if admins want to control the region
    })),
    take(1)
) as Observable<{
    subClaim: string
    accessToken: string
    configToken: string
    region?: AWSRegion
}>

// Mandelbox creation flow
const mandelbox = mandelboxFlow(launchTrigger)

// After the mandelbox flow is done, run the refresh flow so the tokens are being refreshed
// every time but don't impede startup time
const refresh = authRefreshFlow(fromSignal(launchTrigger, mandelbox.success))
merge(refresh.success, refresh.failure).subscribe()

createTrigger(TRIGGER.updateDownloaded, update.downloaded)
createTrigger(TRIGGER.downloadProgress, update.progress)

createTrigger(TRIGGER.authFlowSuccess, auth.success)
createTrigger(TRIGGER.authFlowFailure, auth.failure)
createTrigger(TRIGGER.authRefreshSuccess, refresh.success)

createTrigger(TRIGGER.mandelboxFlowSuccess, mandelbox.success)
createTrigger(TRIGGER.mandelboxFlowFailure, mandelbox.failure)
