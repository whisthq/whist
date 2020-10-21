import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiPost } from "shared/utils/api"
import history from "shared/utils/history"

import * as AuthPureAction from "store/actions/auth/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"

function* emailLogin(action: any) {
    console.log("login saga")
    console.log(action)

    const { json } = yield call(
        apiPost,
        "/account/login",
        {
            username: action.email,
            password: action.password,
        },
        ""
    )

    if (json && json.verified) {
        console.log("cahnging user")
        yield put(
            AuthPureAction.updateUser({
                user_id: action.email,
                name: "",
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
            })
        )
        yield put(
            AuthPureAction.updateAuthFlow({
                loginWarning: "",
            })
        )
    } else {
        console.log("warning")
        yield put(
            AuthPureAction.updateAuthFlow({
                loginWarning: "Invalid username or password. Try again.",
            })
        )
    }
}

function* emailSignup(action: any) {
    const { json, response } = yield call(
        apiPost,
        "/account/register",
        {
            username: action.email,
            password: action.password,
            name: "", // this will need to be changed later!
            feedback: "", //this too!
        },
        ""
    )

    if (json && response.status === 200) {
        AuthPureAction.updateUser({
            user_id: action.email,
            name: "",
            accessToken: json.access_token,
            refreshToken: json.refresh_token,
            emailVerificationToken: json.token,
        })

        console.log(json)

        yield call(sendVerificationEmail, {
            email: action.email,
            token: json.token,
        })

        history.push("/verify")
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                signupWarning: "An error occurred during signup. Try again.",
            })
        )
    }
}

function* sendVerificationEmail(action: any) {
    const state = yield select()
    if (action.email !== "" && action.token !== "") {
        const { json, response } = yield call(
            apiPost,
            "/mail/verification",
            {
                username: action.email,
                token: action.token,
            },
            ""
        )
        const emailsSent = state.AuthReducer.authFlow.verificationEmailsSent
            ? state.AuthReducer.authFlow.verificationEmailsSent
            : 0

        if (json && response.status === 200) {
            yield put(
                AuthPureAction.updateAuthFlow({
                    verificationEmailsSent: emailsSent + 1,
                })
            )
        }
    }
}

export default function* () {
    yield all([
        takeEvery(AuthSideEffect.EMAIL_LOGIN, emailLogin),
        takeEvery(AuthSideEffect.EMAIL_SIGNUP, emailSignup),
    ])
}
