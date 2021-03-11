import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"

import history from "shared/utils/history"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import { updateStripeInfo } from "store/actions/dashboard/payment/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"

import * as api from "shared/api"

function* emailLogin(action: {
    email: string
    password: string
    type: string
}) {
    const { json } = yield call(api.loginEmail, action.email, action.password)

    if (json && json.access_token) {
        yield put(
            updateUser({
                userID: action.email,
                name: json.name,
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                emailVerified: json.verified,
                emailVerificationToken: json.verification_token,
            })
        )

        yield put(
            updateStripeInfo({
                createdTimestamp: json.created_timestamp,
            })
        )

        yield put(
            updateAuthFlow({
                loginWarning: "",
            })
        )
        yield call(fetchPaymentInfo, { email: action.email })

        if (!json.verified) {
            yield call(sendVerificationEmail, {
                email: action.email,
                name: json.name,
                token: json.verification_token,
            })

            history.push("/verify")
        }
    } else {
        yield put(
            updateAuthFlow({
                loginWarning: "Invalid email or password. Try again.",
            })
        )
    }
}

// also used for signup
export function* googleLogin(action: any) {
    if (action.code) {
        var { json, response } = yield call(api.loginGoogle, action.code)
        if (json) {
            if (response.status === 200) {
                yield put(
                    updateUser({
                        userID: json.username,
                        name: json.name, //might want to change this later
                        accessToken: json.access_token,
                        refreshToken: json.refreshToken,
                        emailVerified: true,
                        usingGoogleLogin: json.using_google_login,
                    })
                )

                yield put(
                    updateAuthFlow({
                        loginWarning: "",
                        signupWarning: "",
                    })
                )
            } else if (response.status === 403) {
                yield put(
                    updateAuthFlow({
                        loginWarning: "Try using non-Google login.",
                        signupWarning: "Try using non-Google login.",
                    })
                )
            } else {
                yield put(
                    updateAuthFlow({
                        loginWarning: "Google Login failed. Try another email.",
                        signupWarning:
                            "Google Login failed. Try another email.",
                    })
                )
            }
        } else {
            yield put(
                updateAuthFlow({
                    loginWarning: "Error: No response from Google server.",
                    signupWarning: "Error: No response from Google server.",
                })
            )
        }
    }
}

function* emailSignup(action: {
    email: string
    password: string
    type: string
    name: string
}) {
    const { json, response } = yield call(
        api.signupEmail,
        action.email,
        action.password,
        action.name,
        ""
    )

    if (json && response.status === 200) {
        yield put(
            updateUser({
                userID: action.email,
                name: action.name,
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                emailVerificationToken: json.verification_token,
            })
        )

        yield put(
            updateStripeInfo({
                createdTimestamp: json.created_timestamp,
            })
        )

        yield call(sendVerificationEmail, {
            email: action.email,
            name: action.name,
            token: json.verification_token,
        })

        yield put(
            updateAuthFlow({
                signupWarning: "",
                signupSuccess: true,
            })
        )

        history.push("/verify")
    } else {
        yield put(
            updateAuthFlow({
                signupWarning:
                    "Email already registered. Please log in instead.",
            })
        )
    }
}

export function* sendVerificationEmail(action: any) {
    const state = yield select()
    if (action.email !== "" && action.name !== "" && action.token !== "") {
        const { json, response } = yield call(
            api.emailVerification,
            action.email,
            action.name,
            action.token
        )

        const emailsSent = state.AuthReducer.authFlow.verificationEmailsSent
            ? state.AuthReducer.authFlow.verificationEmailsSent
            : 0

        if (json && response.status === 200) {
            yield put(
                updateAuthFlow({
                    verificationEmailsSent: emailsSent + 1,
                })
            )
        }
    }
}

export function* validateVerificationToken(action: any) {
    const state = yield select()
    const { json, response } = yield call(
        api.validateVerification,
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken,
        state.AuthReducer.user.userID,
        action.token
    )

    if (json && response.status === 200 && json.verified) {
        yield put(
            updateAuthFlow({
                verificationStatus: "success",
            })
        )
        yield delay(2000)
        yield put(
            updateUser({
                emailVerified: true,
            })
        )
    } else {
        yield put(
            updateAuthFlow({
                verificationStatus: "failed",
            })
        )
    }
}

export function* forgotPassword(action: any) {
    const state = yield select()
    const { json } = yield call(
        api.forgotPassword,
        action.username
    )

    const emailsSent = state.AuthReducer.authFlow.forgotEmailsSent
        ? state.AuthReducer.authFlow.forgotEmailsSent
        : 0

    if (json) {
        if (json.token) {
            yield put(
                updateAuthFlow({
                    token: json.token,
                    url: json.url,
                })
            )
        }
        if (json.verified) {
            yield put(
                updateAuthFlow({
                    forgotStatus: "We've sent you a password reset email",
                    forgotEmailsSent: emailsSent + 1,
                })
            )
        } else {
            yield put(
                updateAuthFlow({
                    forgotStatus: "Error: Your email is not verified",
                    forgotEmailsSent: emailsSent + 1,
                })
            )
        }
    } else {
        yield put(
            updateAuthFlow({
                forgotStatus: "Error: Server is unresponsive",
                forgotEmailsSent: emailsSent + 1,
            })
        )
    }
}

export function* validateResetToken(action: any) {
    const state = yield select()
    const { json } = yield call(
        api.validateVerification, 
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken,
        state.AuthReducer.authFlow.passwordResetEmail,
        action.token
    )

    // at some later point in time we may find it helpful to change strings here to some sort of enum
    if (json && json.status === 200) {
        yield put(
            updateAuthFlow({
                resetTokenStatus: "verified",
                passwordResetToken: action.token,
            })
        )
    } else {
        // this happens when the server 500s or token is invalid/expired
        yield put(
            updateAuthFlow({
                resetTokenStatus: "invalid",
            })
        )
    }
}

export function* resetPassword(action: any) {   
    const state = yield select()

    const { response } = yield call(
        api.resetPassword,
        state.AuthReducer.authFlow.passwordResetToken,
        state.AuthReducer.authFlow.passwordResetEmail,
        action.password
    )

    // TODO do something with the response
    if (response && response.status === 200) {
        // TODO: Remove the resetDone state
        yield put(
            updateAuthFlow({
                resetDone: true,
                passwordResetEmail: null,
                passwordResetToken: null,
            })
        )
    } else {
        yield put(
            updateAuthFlow({
                resetDone: false,
                passwordResetEmail: null,
                passwordResetToken: null,
            })
        )
    }
}

export function* updatePassword(action: any) {
    const state = yield select()

    const { json } = yield call(
        api.passwordUpdate,
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.userID,
        action.currentPassword
    )

    if (json && json.status) {
        if (json.status === 200) {
            yield put(
                updateAuthFlow({
                    passwordVerified: "success",
                })
            )
            yield call(resetPassword, {
                password: action.newPassword,
                token: state.AuthReducer.user.accessToken,
                username: state.AuthReducer.user.userID,
            })
        } else {
            yield put(
                updateAuthFlow({
                    passwordVerified: "failed",
                })
            )
        }
    }
}

function* fetchPaymentInfo(action: any) {
    const state = yield select()
    const { json } = yield call(
        api.stripePaymentInfo,
        state.AuthReducer.user.accessToken,
        action.email
    )
    if (json) {
        yield put(
            updateStripeInfo({
                cardBrand: json.source ? json.source.card.brand : null,
                cardLastFour: json.source ? json.source.card.last4 : null,
                postalCode: json.source
                    ? json.source.owner.address.postal_code
                    : null,
                plan: json.subscription
                    ? json.subscription.plan.metadata.name
                    : null,
            })
        )
    }
}

export default function* authSaga() {
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
        takeEvery(
            AuthSideEffect.SEND_VERIFICATION_EMAIL,
            sendVerificationEmail
        ),
        takeEvery(AuthSideEffect.UPDATE_PASSWORD, updatePassword),
        takeEvery(AuthSideEffect.FETCH_PAYMENT_INFO, fetchPaymentInfo),
    ])
}
