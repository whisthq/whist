import { screen } from "electron"
import { merge, Observable, zip } from "rxjs"
import { map } from "rxjs/operators"
import { mandelboxCreateFlow } from "@app/main/flows/mandelbox/create"
import { hostSpinUpFlow } from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { AWSRegion } from "@app/@types/aws"

export const mandelboxFlow = flow<{
    subClaim: string
    accessToken: string
    configToken: string
    region?: AWSRegion
}>("mandelboxFlow", (trigger) => {
    trigger.subscribe((x) => console.log("mSubscribe", x))
    const dpiStream = trigger.pipe(
        map(() => Math.round(screen.getPrimaryDisplay().scaleFactor * 96))
    )

    const create = mandelboxCreateFlow(
        zip(trigger, dpiStream).pipe(
            map(([t, dpi]) => ({
                subClaim: t.subClaim,
                accessToken: t.accessToken,
                dpi: dpi,
                region: t.region,
            }))
        )
    )

    const host = hostSpinUpFlow(
        zip([trigger, create.success, dpiStream]).pipe(
            map(([t, c, dpi]) => ({
                ip: c.ip,
                dpi: dpi,
                user_id: t.subClaim,
                config_encryption_token: t.configToken,
                jwt_access_token: t.accessToken,
                mandelbox_id: c.mandelboxID,
            }))
        )
    )

    return {
        success: host.success,
        failure: merge(create.failure, host.failure),
    }
})
