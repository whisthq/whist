import { put, takeEvery, all, call, select } from "redux-saga/effects"

import { apiPost, graphQLPost } from "shared/utils/api"
import history from "shared/utils/history"

import * as AuthPureAction from "store/actions/auth/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import {
    UPDATE_WAITLIST_AUTH_EMAIL,
    UPDATE_WAITLIST,
} from "shared/constants/graphql"
import { SIGNUP_POINTS } from "shared/utils/points"

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

    console.log(json)

    if (json && json.access_token) {
        yield put(
            AuthPureAction.updateUser({
                user_id: action.email,
                name: action.email, // temp so it can be displayed
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                emailVerified: json.verified,
            })
        )
        yield put(
            AuthPureAction.updateAuthFlow({
                loginWarning: "",
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

// also used for signup
function* googleLogin(action: any) {
    const state = yield select()

    if (action.code) {
        var { json, response } = yield call(
            apiPost,
            "/google/login",
            {
                code: action.code,
            },
            ""
        )
        if (json) {
            if (response.status === 200) {
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
                        loginWarning: "",
                        signupWarning: "",
                    })
                )

                yield call(
                    graphQLPost,
                    UPDATE_WAITLIST_AUTH_EMAIL,
                    "UpdateWaitlistAuthEmail",
                    {
                        user_id: state.WaitlistReducer.waitlistUser.user_id,
                        authEmail: json.username,
                    }
                )

                yield call(graphQLPost, UPDATE_WAITLIST, "UpdateWaitlist", {
                    user_id: state.WaitlistReducer.waitlistUser.user_id,
                    points:
                        state.WaitlistReducer.waitlistUser.points +
                        SIGNUP_POINTS,
                    referrals: state.WaitlistReducer.waitlistUser.referrals,
                })
            } else if (response.status === 403) {
                yield put(
                    AuthPureAction.updateAuthFlow({
                        loginWarning: "Try using non-Google login.",
                        signupWarning: "Try using non-Google login.",
                    })
                )
            } else {
                yield put(
                    AuthPureAction.updateAuthFlow({
                        loginWarning: "Google Login failed. Try another email.",
                        signupWarning:
                            "Google Login failed. Try another email.",
                    })
                )
            }
        } else {
            yield put(
                AuthPureAction.updateAuthFlow({
                    loginWarning: "Error: No response from Google server.",
                    signupWarning: "Error: No response from Google server.",
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

        yield call(sendVerificationEmail, {
            email: action.email,
            token: json.token,
        })

        yield put(
            AuthPureAction.updateAuthFlow({
                signupWarning: "",
                signupSuccess: true,
            })
        )

        history.push("/verify")
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                signupWarning:
                    "Email already registered. Please log in instead.",
            })
        )
    }
}

function* sendVerificationEmail(action: any) {
    console.log(action)
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
    console.log("IN VALIDATE SAGA")
    const { json, response } = yield call(
        apiPost,
        "/account/verify",
        {
            username: state.AuthReducer.user.user_id,
            token: action.token,
        },
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken
    )

    if (json && response.status === 200 && json.verified) {
        yield put(
            AuthPureAction.updateUser({
                emailVerified: true,
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
    } else {
        // this happens when the server 500s basically
        yield put(
            AuthPureAction.updateAuthFlow({
                resetTokenStatus: "invalid",
            })
        )
    }
}

function* resetPassword(action: any) {
    // const state = yield select()

    console.log(action)

    yield call(
        apiPost,
        "/account/update",
        {
            username: action.username,
            password: action.password,
        },
        action.token
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
        takeEvery(
            AuthSideEffect.SEND_VERIFICATION_EMAIL,
            sendVerificationEmail
        ),
    ])
}
