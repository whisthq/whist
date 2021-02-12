import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiGet } from "shared/utils/general/api"
import { FractalAPI } from "shared/types/api"
import { FractalAuthCache } from "shared/types/cache"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import * as AuthAction from "store/actions/auth/sideEffects"

function* validateAccessToken(action: { accessToken: string }) {
    /*
        Description:
            Validates an access token used to login a user, then call functions to fetch
            external and connected app data
        Arguments:
            action (Record): Redux action containing access token
    */

    const state = yield select()

    const { json, success } = yield call(
        apiGet,
        FractalAPI.TOKEN.VALIDATE,
        action.accessToken
    )

    if (success && json.user) {
        const Store = require("electron-store")
        const storage = new Store()

        storage.set(FractalAuthCache.ACCESS_TOKEN, action.accessToken)

        yield put(
            updateUser({
                userID: json.user.user_id,
                accessToken: action.accessToken,
                refreshToken: json.user.refresh_token,
                name: json.user.name,
            })
        )
    } else {
        yield put(
            updateAuthFlow({
                failures: state.AuthReducer.authFlow.failures + 1,
                loginToken: state.AuthReducer.authFlow.loginToken,
            })
        )
    }
}

export default function* authSaga() {
    yield all([
        takeEvery(AuthAction.VALIDATE_ACCESS_TOKEN, validateAccessToken),
    ])
}
