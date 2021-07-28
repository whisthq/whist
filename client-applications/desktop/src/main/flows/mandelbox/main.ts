import { screen } from "electron"
import { head } from "lodash"
import { merge, zip } from "rxjs"
import { map } from "rxjs/operators"
import { mandelboxCreateFlow } from "@app/main/flows/mandelbox/create"
import { authRefreshFlow } from "@app/main/flows/auth"
import { hostSpinUpFlow } from "@app/main/flows/mandelbox/host"
import { flow } from "@app/utils/flows"
import { AWSRegion } from "@app/@types/aws"

export const mandelboxFlow = flow<{
    subClaim: string
    accessToken: string
    refreshToken: string
    configToken: string
    region?: AWSRegion
}>("mandelboxFlow", (trigger) => {
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

    const refreshedAuthInfo = authRefreshFlow(
        zip([trigger, create.expired]).pipe(map(head))
    )

    const retry = mandelboxCreateFlow(
        zip([refreshedAuthInfo.success, trigger]).pipe(
            map(([authInfo, { region }]) => ({
                ...authInfo,
                region: region,
            }))
        )
    )

    const host = hostSpinUpFlow(
        zip([trigger, merge(create.success, retry.success), dpiStream]).pipe(
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
        failure: merge(create.failure, refreshedAuthInfo.failure, host.failure),
    }
})
