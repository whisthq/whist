import { put, takeEvery, all, call, select, delay } from "redux-saga/effects"

import history from "shared/utils/history"
import {
    decryptConfigToken,
    encryptConfigToken,
    generateConfigToken,
} from "shared/utils/helpers"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import { updateStripeInfo } from "store/actions/dashboard/payment/pure"
import * as AuthSideEffect from "store/actions/auth/sideEffects"

import * as api from "shared/api"

function* emailLogin(action: {
    email: string
    password: string
    rememberMe?: boolean
    type: string
}) {
    /*
        Function that calls the /account/login endpoint on the webserver to try and login to the website.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            email (string): email to login with
            password (string): password to login with
            rememberMe (boolean): whether the user wants to stay logged in (deprecated)
            type (string): identifier for this side effect
    */
    const { json } = yield call(api.loginEmail, action.email, action.password)

    if (json && json.access_token) {
        const encryptedConfigToken = json.encrypted_config_token
        const configToken = decryptConfigToken(
            encryptedConfigToken,
            action.password
        )
        yield put(
            updateUser({
                userID: action.email,
                name: json.name,
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                configToken: configToken,
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
export function* googleLogin(action: {
    code: any
    rememberMe?: boolean
    type: string
}) {
    /*
        Function that logs in using a user's Google account (unused)
    */
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
    rememberMe?: boolean
    type: string
    name: string
}) {
    /*
        Function that calls the /account/register endpoint on the webserver to try and register a new user.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            email (string): email to signup with
            password (string): password to signup with
            rememberMe (boolean): whether the user wants to stay logged in (deprecated)
            type (string): identifier for this side effect
            name (string): name of the user
    */
    const configToken = yield call(generateConfigToken())
    const encryptedConfigToken = encryptConfigToken(
        configToken,
        action.password
    )
    const { json, response } = yield call(
        api.signupEmail,
        action.email,
        action.password,
        action.name,
        "",
        encryptedConfigToken
    )

    if (json && response.status === 200) {
        yield put(
            updateUser({
                userID: action.email,
                name: action.name,
                accessToken: json.access_token,
                refreshToken: json.refresh_token,
                configToken: configToken,
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

export function* sendVerificationEmail(action: any): any {
    /*
        Function that calls the /mail endpoint on the webserver to send a verification email to the user.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            email (string): email to login with
            name (string): name of the user
            token (string): email verification token of the user (received from webserver on login/signup)
            type (string): identifier for this side effect
    */
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

export function* validateVerificationToken(action: {
    token: string
    type: string
}): any {
    /*
        Function that calls the /account/verify endpoint on the webserver to try and verify a user's email verification token.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            token (string): email verification token to verify
            type (string): identifier for this side effect
    */
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

export function* forgotPassword(action: {
    username: string
    token: string
    type: string
}): any {
    /*
        Function that calls the /mail endpoint on the webserver to send a password forgot/reset email.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            username (string): email of the user
            token (string): access (? email verification?) token for the user // check after Ming's auth refactor
            type (string): identifier for this side effect
    */
    const state = yield select()
    const { json } = yield call(
        api.passwordForgot,
        action.username,
        action.token
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
                    forgotStatus: "Email sent",
                    forgotEmailsSent: emailsSent + 1,
                })
            )
        } else {
            yield put(
                updateAuthFlow({
                    forgotStatus: "Not verified",
                    forgotEmailsSent: emailsSent + 1,
                })
            )
        }
    } else {
        yield put(
            updateAuthFlow({
                forgotStatus: "No response",
                forgotEmailsSent: emailsSent + 1,
            })
        )
    }
}

export function* validateResetToken(action: { token: string; type: string }) {
    /*
        Function that calls the /token/validate endpoint on the webserver to verify the token that was passed through the "Forgot Password" url
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            token (string): access token of the user
            type (string): identifier for this side effect
    */
    yield select()
    const { json } = yield call(api.validatePasswordReset, action.token)

    // at some later point in time we may find it helpful to change strings here to some sort of enum
    if (json && json.status === 200) {
        yield put(
            updateAuthFlow({
                resetTokenStatus: "verified",
                passwordResetEmail: json.user.user_id, // changed from userID
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

export function* resetPassword(action: {
    token: string
    username: string
    password: string
    type: string
}) {
    /*
        Function that calls the /account/update endpoint on the webserver to try and change an existing user's password.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            username (string): user email
            password (string): password to change to
            token (string): user's access token
            type (string): identifier for this side effect
    */
    const configToken = yield call(generateConfigToken())
    const encryptedConfigToken = encryptConfigToken(
        configToken,
        action.password
    )
    const { response } = yield call(
        api.passwordReset,
        action.token,
        action.username,
        action.password,
        encryptedConfigToken
    )

    // TODO do something with the response https://github.com/fractal/website/issues/334
    if (response && response.status === 200) {
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

export function* updatePassword(action: {
    currentPassword: string
    newPassword: string
    type: string
}): any {
    /*
        Function that calls the /account/verify_password endpoint to verify that the currently logged-in user is valid,
        then uses the /account/update endpoint on the webserver to try and change the existing user's password. Assumes that the user 
        remembers their old password.

        It also processes the returned json and updates the state of the application as necessary.

        Args:
            currentPassword (string): current password
            newPassword (string): password to change to
            type (string): identifier for this side effect
    */
    const state = yield select()

    const { json } = yield call(
        api.passwordVerify,
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

            const configToken = state.AuthReducer.user.configToken
            const encryptedConfigToken = encryptConfigToken(
                configToken,
                action.newPassword
            )
            const { response } = yield call(
                api.passwordReset,
                state.AuthReducer.user.accessToken,
                state.AuthReducer.user.userID,
                action.newPassword,
                encryptedConfigToken
            )

            // TODO do something with the response https://github.com/fractal/website/issues/334
            if (response && response.status === 200) {
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
        } else {
            yield put(
                updateAuthFlow({
                    passwordVerified: "failed",
                })
            )
        }
    }
}

function* fetchPaymentInfo(action: any): any {
    /*
        Function that calls the /stripe/retrieve endpoint on the webserver to retrieve the user's payment information.
        It also processes the returned json and updates the state of the application as necessary.

        Args:
            email (string): user's email
            type (string): identifier for this side effect
    */
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
        takeEvery(AuthSideEffect.FORGOT_PASSWORD, forgotPassword),
        takeEvery(
            AuthSideEffect.SEND_VERIFICATION_EMAIL,
            sendVerificationEmail
        ),
        takeEvery(AuthSideEffect.UPDATE_PASSWORD, updatePassword),
        takeEvery(AuthSideEffect.FETCH_PAYMENT_INFO, fetchPaymentInfo),
    ])
}
