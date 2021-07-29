import { from, zip, merge } from "rxjs"
import { switchMap, map, filter, tap } from "rxjs/operators"

import { flow, fork } from "@app/utils/flows"
import {
    generateRefreshedAuthInfo,
    generateRandomConfigToken,
    authInfoValid,
    isExpired
} from "@app/utils/auth"
import { store } from "@app/utils/persist"

export const authRefreshFlow = flow<{
    userEmail: string
    subClaim: string
    accessToken: string
    refreshToken: string
}>("authFlow", (trigger) => {
    const expired = trigger.pipe(filter((tokens) => isExpired(tokens.accessToken)))
    const notExpired = trigger.pipe(filter((tokens) => !isExpired(tokens.accessToken)))

    const refreshed = expired.pipe(
        switchMap((tokens) =>
            from(generateRefreshedAuthInfo(tokens.refreshToken))
        )
    )

    return {
        success: merge(notExpired, refreshed).pipe(filter((result) => authInfoValid(result))),
        failure: merge(notExpired, refreshed).pipe(filter((result) => !authInfoValid(result))),
    }
})

export default flow<{
    userEmail: string
    subClaim: string
    accessToken: string
    refreshToken: string
    configToken?: string
}>("authFlow", (trigger) => {
    const refreshedAuthInfo = authRefreshFlow(trigger)

    const authInfoWithConfig = merge(refreshedAuthInfo.success, refreshedAuthInfo.failure).pipe(
        map((tokens) => ({
            ...tokens,
            configToken:
                tokens.configToken ??
                store.get("configToken") ??
                generateRandomConfigToken(),
        }))
    )

    return fork(authInfoWithConfig, {
        success: (result: any) => authInfoValid(result),
        failure: (result: any) => !authInfoValid(result),
    })
})
