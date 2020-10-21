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
        console.log("changing user")
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

function* validateVerificationToken(action: any) {
    const state = yield select()
    const { json, response } = yield call(
        apiPost,
        "/account/verify",
        {
            username: state.AuthReducer.username,
            token: action.token,
        },
        state.AuthReducer.tokens.access_token
    )

    const attemptsExecuted = state.AuthReducer.authFlow
        .verificationAttemptsExecuted
        ? state.AuthReducer.authFlow.verificationAttemptsExecuted
        : 0

    if (json && response.status === 200 && json.verified) {
        yield put(
            AuthPureAction.updateUser({
                emailVerified: true,
            })
        )
        yield put(
            AuthPureAction.updateAuthFlow({
                verificationAttemptsExecuted: attemptsExecuted + 1,
            })
        )
    } else {
        yield put(
            AuthPureAction.updateUser({
                emailVerified: false,
            })
        )
        yield put(
            AuthPureAction.updateAuthFlow({
                verificationAttemptsExecuted: attemptsExecuted + 1,
            })
        )
    }
}

function* validateResetToken(action: any) {
    yield select()
    const { json } = yield call(
        apiPost,
        "/token/validate",
        {
            token: action.token,
        },
        ""
    )
    // at some later point in time we may find it helpful to change strings here to some sort of enum
    if (json) {
        if (json.status === 200) {
            yield put(
                AuthPureAction.updateAuthFlow({
                    resetTokenStatus: "verified",
                })
            )
        } else {
            if (json.error === "Expired token") {
                yield put(
                    AuthPureAction.updateAuthFlow({
                        resetTokenStatus: "expired",
                    })
                )
            } else {
                yield put(
                    AuthPureAction.updateAuthFlow({
                        resetTokenStatus: "invalid",
                    })
                )
            }
        }
    }
}

export default function* () {
    yield all([
        takeEvery(AuthSideEffect.EMAIL_LOGIN, emailLogin),
        takeEvery(AuthSideEffect.EMAIL_SIGNUP, emailSignup),
        takeEvery(
            AuthSideEffect.VALIDATE_VERIFY_TOKEN,
            validateVerificationToken
        ),
        takeEvery(AuthSideEffect.VALIDATE_RESET_TOKEN, validateResetToken),
    ])
}
