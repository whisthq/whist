import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import { pick } from "lodash"

import mandelboxCreateFlow from "@app/main/flows/mandelbox/create"
import mandelboxPollingFlow from "@app/main/flows/mandelbox/polling"
import hostServiceFlow from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { AWSRegion } from "@app/@types/aws"
import { fromSignal } from "@app/utils/observables"

export default flow(
    "mandelboxFlow",
    (
        trigger: Observable<{
            subClaim: string
            accessToken: string
            configToken: string
            region?: AWSRegion
        }>
    ) => {
        const create = mandelboxCreateFlow(
            trigger.pipe(
                map((t) => pick(t, ["subClaim", "accessToken", "region"]))
            )
        )

        const polling = mandelboxPollingFlow(
            zip(create.success, trigger).pipe(
                map(([c, t]) => ({
                    ...pick(c, ["mandelboxID"]),
                    ...pick(t, ["accessToken"]),
                }))
            )
        )

        const host = hostServiceFlow(
            zip([trigger, create.success]).pipe(
                map(([t, _c]) =>
                    pick(t, ["subClaim", "accessToken", "configToken"])
                )
            )
        )

        return {
            success: fromSignal(polling.success, host.success),
            failure: merge(create.failure, polling.failure, host.failure),
        }
    }
)
