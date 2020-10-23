import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiPost } from "shared/utils/api"
import history from "shared/utils/history"

import * as AuthPureAction from "store/actions/auth/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"

function* emailLogin(action: any) {
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
                name: action.email, // temp so it can be displayed
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
            })
        )
        yield put(
            AuthPureAction.updateAuthFlow({
                loginStatus: "",
            })
        )
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                loginStatus: "Invalid username or password. Try again.",
            })
        )
    }
}

// also used for signup
function* googleLogin(action: any) {
    yield select()

    if (action.code) {
        const { json } = yield call(
            apiPost,
            "/google/login",
            {
                code: action.code,
            },
            ""
        )
        if (json) {
            if (json.status === 200) {
                yield put(
                    AuthPureAction.updateUser({
                        user_id: json.username,
                        name: json.username, //might want to change this later
                        accessToken: json.access_token,
                        refreshToken: json.refreshToken,
                        emailVerified: true,
                    })
                )

                yield put(
                    AuthPureAction.updateAuthFlow({
                        googleLoginStatus: 200,
                        loginStatus: "",
                        signupStatus: "",
                    })
                )
            } else {
                yield put(
                    AuthPureAction.updateAuthFlow({
                        googleLoginStatus: json.status,
                        loginStatus: "Failed to Authenticate With Google",
                        signupStatus: "Failed to Authenticate With Google",
                    })
                )
            }
        } else {
            yield put(
                AuthPureAction.updateAuthFlow({
                    googleLoginStatus: null,
                    loginStatus: "No Response from Server for Google Login",
                    signupStatus: "No Response from Server for Google Login",
                })
            )
        }
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
        yield put(
            AuthPureAction.updateUser({
                user_id: action.email,
                name: action.email, // also temp so it can display
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                emailVerificationToken: json.token,
            })
        )

        console.log(json)

        yield call(sendVerificationEmail, {
            email: action.email,
            token: json.token,
        })

        yield put(
            AuthPureAction.updateAuthFlow({
                signupStatus: "",
            })
        )

        history.push("/verify")
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                signupStatus: "An error occurred during signup. Try again.",
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
            username: state.AuthReducer.user.user_id,
            token: action.token,
        },
        state.AuthReducer.user.accessToken
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

function* forgotPassword(action: any) {
    const state = yield select()
    const { json } = yield call(
        apiPost,
        "/mail/forgot",
        {
            username: action.username,
        },
        ""
    )

    const emailsSent = state.AuthReducer.authFlow.forgotEmailsSent
        ? state.AuthReducer.authFlow.forgotEmailsSent
        : 0

    if (json) {
        if (json.verified) {
            yield put(
                AuthPureAction.updateAuthFlow({
                    forgotStatus: "Email sent",
                    forgotEmailsSent: emailsSent + 1,
                })
            )
        } else {
            yield put(
                AuthPureAction.updateAuthFlow({
                    forgotStatus: "Not verified",
                    forgotEmailsSent: emailsSent + 1,
                })
            )
        }
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                forgotStatus: "No response",
                forgotEmailsSent: emailsSent + 1,
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
    console.log(`here comes the validation: ${JSON.stringify(json)}`)
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

function* resetPassword(action: any) {
    yield select()

    yield call(
        apiPost,
        "/account/resetPassword",
        {
            username: action.username,
            password: action.password,
        },
        ""
    )

    // TODO do something with the response
    yield put(
        AuthPureAction.updateAuthFlow({
            resetDone: true,
        })
    )
}

export default function* () {
    yield all([
        takeEvery(AuthSideEffect.EMAIL_LOGIN, emailLogin),
        takeEvery(AuthSideEffect.EMAIL_SIGNUP, emailSignup),
        takeEvery(AuthSideEffect.GOOGLE_LOGIN, googleLogin),
        takeEvery(
            AuthSideEffect.VALIDATE_VERIFY_TOKEN,
            validateVerificationToken
        ),
        takeEvery(AuthSideEffect.VALIDATE_RESET_TOKEN, validateResetToken),
        takeEvery(AuthSideEffect.RESET_PASSWORD, resetPassword),
        takeEvery(AuthSideEffect.FORGOT_PASSWORD, forgotPassword),
    ])
}
