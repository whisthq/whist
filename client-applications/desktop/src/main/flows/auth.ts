import { has } from "lodash"
import { from, zip } from "rxjs"
import { switchMap, map, filter, share } from "rxjs/operators"

import { flow } from "@app/utils/flows"
import {
    generateRandomConfigToken,
    authInfoRefreshRequest,
    authInfoParse,
} from "@fractal/core-ts"
import { store } from "@app/utils/persist"

export default flow<{
    userEmail: string
    subClaim: string
    accessToken: string
    refreshToken: string
    configToken?: string
}>("authFlow", (trigger) => {
    const refreshedAuthInfo = trigger.pipe(
        switchMap((tokens: { refreshToken: string }) =>
            from(authInfoRefreshRequest(tokens))
        ),
        map(authInfoParse),
        share()
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

    return {
        failure: authInfoWithConfig.pipe(
            filter((tokens) => has(tokens, "error"))
        ),
        success: authInfoWithConfig.pipe(
            filter((tokens) => !has(tokens, "error"))
        ),
    }
})
