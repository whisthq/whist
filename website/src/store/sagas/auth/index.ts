import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"

import { apiPost, graphQLPost } from "shared/utils/api"
import history from "shared/utils/history"

import * as AuthPureAction from "store/actions/auth/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"
import {
    UPDATE_WAITLIST_AUTH_EMAIL,
    UPDATE_WAITLIST_REFERRALS,
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

    if (json && json.access_token) {
        yield put(
            AuthPureAction.updateUser({
                user_id: action.email,
                name: json.name,
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                emailVerified: json.verified,
                canLogin: json.can_login,
                emailVerificationToken: json.verification_token,
                cardBrand: json.card_brand,
                cardLastFour: json.card_last_four,
                postalCode: json.postal_code,
            })
        )
        yield put(
            AuthPureAction.updateAuthFlow({
                loginWarning: "",
            })
        )

        if (!json.verified) {
            yield call(sendVerificationEmail, {
                email: action.email,
                token: json.verification_token,
            })

            history.push("/verify")
        }
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
                        name: json.name, //might want to change this later
                        accessToken: json.access_token,
                        refreshToken: json.refreshToken,
                        emailVerified: true,
                        canLogin: json.can_login,
                        usingGoogleLogin: json.using_google_login,
                        cardBrand: json.card_brand,
                        cardLastFour: json.card_last_four,
                        postalCode: json.postal_code,
                    })
                )

                yield put(
                    AuthPureAction.updateAuthFlow({
                        loginWarning: "",
                        signupWarning: "",
                    })
                )

                if (state.WaitlistReducer.waitlistUser.user_id) {
                    yield call(
                        graphQLPost,
                        UPDATE_WAITLIST_AUTH_EMAIL,
                        "UpdateWaitlistAuthEmail",
                        {
                            user_id: state.WaitlistReducer.waitlistUser.user_id,
                            authEmail: json.username,
                        }
                    )

                    yield call(
                        graphQLPost,
                        UPDATE_WAITLIST_REFERRALS,
                        "UpdateWaitlistReferrals",
                        {
                            user_id: state.WaitlistReducer.waitlistUser.user_id,
                            points:
                                state.WaitlistReducer.waitlistUser.points +
                                SIGNUP_POINTS,
                            referrals:
                                state.WaitlistReducer.waitlistUser.referrals,
                        }
                    )
                }
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
            name: action.name,
            feedback: "", //this will need to be changed later!
        },
        ""
    )

    if (json && response.status === 200) {
        yield put(
            AuthPureAction.updateUser({
                user_id: action.email,
                name: action.name,
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                emailVerificationToken: json.verification_token,
            })
        )

        yield call(sendVerificationEmail, {
            email: action.email,
            token: json.verification_token,
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
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken
    )

    if (json && response.status === 200 && json.verified) {
        yield put(
            AuthPureAction.updateAuthFlow({
                verificationStatus: "success",
            })
        )
        yield delay(2000)
        yield put(
            AuthPureAction.updateUser({
                emailVerified: true,
            })
        )
        if (state.WaitlistReducer.waitlistUser.user_id) {
            yield call(
                graphQLPost,
                UPDATE_WAITLIST_REFERRALS,
                "UpdateWaitlistReferrals",
                {
                    user_id: state.WaitlistReducer.waitlistUser.user_id,
                    points:
                        state.WaitlistReducer.waitlistUser.points +
                        SIGNUP_POINTS,
                    referrals: state.WaitlistReducer.waitlistUser.referrals,
                }
            )
        }
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                verificationStatus: "failed",
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
    if (json) {
        if (json.status === 200) {
            yield put(
                AuthPureAction.updateAuthFlow({
                    resetTokenStatus: "verified",
                    passwordResetEmail: json.user,
                    passwordResetToken: action.token,
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
    const { response } = yield call(
        apiPost,
        "/account/update",
        {
            username: action.username,
            password: action.password,
        },
        action.token
    )

    // TODO do something with the response
    if (response && response.status === 200) {
        yield put(
            AuthPureAction.updateAuthFlow({
                resetDone: true,
                passwordResetEmail: null,
                passwordResetToken: null,
            })
        )
    } else {
        yield put(
            AuthPureAction.updateAuthFlow({
                resetDone: false,
                passwordResetEmail: null,
                passwordResetToken: null,
            })
        )
    }
}

function* updatePassword(action: any) {
    const state = yield select()

    const { json } = yield call(
        apiPost,
        "/account/verify_password",
        {
            username: state.AuthReducer.user.user_id,
            password: action.currentPassword,
        },
        state.AuthReducer.user.accessToken
    )

    if (json && json.status) {
        if (json.status === 200) {
            yield put(
                AuthPureAction.updateAuthFlow({
                    passwordVerified: "success",
                })
            )
            yield call(resetPassword, {
                username: state.AuthReducer.user.user_id,
                password: action.newPassword,
                token: state.AuthReducer.user.accessToken,
            })
        } else {
            yield put(
                AuthPureAction.updateAuthFlow({
                    passwordVerified: "failed",
                })
            )
        }
    }
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
        takeEvery(AuthSideEffect.UPDATE_PASSWORD, updatePassword),
    ])
}
