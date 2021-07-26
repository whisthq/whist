import { from, zip } from "rxjs"
import { switchMap, map } from "rxjs/operators"

import { flow, fork } from "@app/utils/flows"
import {
    generateRefreshedAuthInfo,
    generateRandomConfigToken,
    authInfoValid,
} from "@app/utils/auth"
import { store } from "@app/utils/persist"

export const authFlow = flow<{
    userEmail: string
    subClaim: string
    accessToken: string
    refreshToken: string
    configToken?: string
}>("authFlow", (trigger) => {
    const refreshedAuthInfo = trigger.pipe(
        switchMap((tokens: { refreshToken: string }) =>
            from(generateRefreshedAuthInfo(tokens.refreshToken))
        )
    )

    const authInfoWithConfig = zip(refreshedAuthInfo, trigger).pipe(
        map(([authInfo, tokens]) => ({
            ...authInfo,
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
