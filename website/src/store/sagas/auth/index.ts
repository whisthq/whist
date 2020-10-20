import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiPost } from "shared/utils/api"
import * as AuthPureAction from "store/actions/auth/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"

function* emailLogin(action: any) {
    const { json } = yield call(
        apiPost,
        "/account/login",
        {
            username: action.email,
            password: action.password,
            name: "", // this will need to be changed later!
            feedback: "", //this too!
        },
        ""
    )

    if (json && json.verified) {
        yield put(
            AuthPureAction.updateUser({
                user_id: action.email,
                name: "",
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
            })
        )
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                loginWarning: "Invalid username or password. Try again.",
            })
        )
    }
}

export default function* () {
    yield all([takeEvery(AuthSideEffect.EMAIL_LOGIN, emailLogin)])
}
