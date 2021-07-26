import { merge, negate } from "lodash"
import { from, zip } from "rxjs"
import { switchMap, map, filter } from "rxjs/operators"
import { flow } from "@app/utils/flows"
import {
    generateRefreshedAuthInfo,
    generateRandomConfigToken,
    authInfoValid,
} from "@app/utils/auth"
import { store } from "@app/utils/persist"

export const authConfigTokenFlow = flow<{ configToken?: string }>(
    "authConfigTokenFlow",
    (trigger) => {
        const token = trigger.pipe(
            map((tokens) => ({
                configToken:
                    tokens.configToken ??
                    store.get("configToken") ??
                    generateRandomConfigToken(),
            }))
        )

        return { success: token }
    }
)

export const authRefreshFlow = flow<{
    refreshToken: string
}>("authFlow", (trigger) => {
    const info = trigger.pipe(
        switchMap((tokens: { refreshToken: string }) =>
            from(generateRefreshedAuthInfo(tokens.refreshToken))
        )
    )

    return {
        success: info.pipe(filter((result) => authInfoValid(result))),
        failure: info.pipe(filter((result) => !authInfoValid(result))),
    }
})

export const authFlow = flow<{
    userEmail: string
    subClaim: string
    accessToken: string
    refreshToken: string
    configToken?: string
}>("authFlow", (trigger) => {
    const configToken = authConfigTokenFlow(trigger)

    const authWithConfig = zip([trigger, configToken.success]).pipe(
        map((args) => merge(...args))
    )

    return {
        success: authWithConfig.pipe(filter(authInfoValid)),
        failure: authWithConfig.pipe(filter(negate(authInfoValid))),
    }
})
