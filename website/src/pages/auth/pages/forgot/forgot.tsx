import React, {
    useState,
    useEffect,
    KeyboardEvent,
    ChangeEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"

import Input from "shared/components/input"
import FractalKey from "shared/types/input"
import { AuthFlow } from "shared/types/reducers"

import * as AuthSideEffect from "store/actions/auth/sideEffects"
import * as AuthPureAction from "store/actions/auth/pure"

import { checkEmail } from "pages/auth/shared/helpers/authHelpers"
import AuthNavigator from "pages/auth/shared/components/authNavigator"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"
import PLACEHOLDER from "shared/constants/form"
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthButton from "pages/auth/shared/components/authButton"
import {routeMap, fractalRoute} from "shared/constants/routes"
import history from "shared/utils/history"

const Forgot = (props: {
    dispatch: Dispatch<any>
    authFlow: AuthFlow
    emailToken?: string
}) => {
    /*
        Component for when the user forgets their login information.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
    */
    const { authFlow, emailToken, dispatch } = props

    const [email, setEmail] = useState("")
    const [processing, setProcessing] = useState(false)
    const [gotResponse, setGotResponse] = useState(false)

    const backToLogin = () => {
        history.push(fractalRoute(routeMap.AUTH))
    }

    const forgot = () => {
        if (checkEmail(email)) {
            setProcessing(true)
            dispatch(
                AuthPureAction.updateAuthFlow({
                    passwordResetEmail: email,
                })
            )
            dispatch(AuthSideEffect.forgotPassword(email, emailToken))
        }
    }

    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            forgot() // note check happens inside forgot
        }
    }

    useEffect(() => {
        if (authFlow.forgotStatus && authFlow.forgotEmailsSent) {
            setProcessing(false)
            setGotResponse(true)
        }
    }, [authFlow.forgotStatus, authFlow.forgotEmailsSent])

    // useEffect(() => {
    //     setGotResponse(false)
    // }, [])

    if (processing) {
        return (
            <div data-testid={AUTH_IDS.LOADING}>
                <PuffAnimation />
            </div>
        )
    } else if (gotResponse) {
        // for testing purposes, only to get the reset password token to navigate
        if (emailToken && emailToken?.length > 0) {
            return (
                <div>
                    <p data-testid={AUTH_IDS.RESETTOKEN}>{authFlow.token}</p>
                    <p data-testid={AUTH_IDS.RESETURL}>{authFlow.url}</p>
                </div>
            )
        }
        return (
            <AuthContainer title={authFlow.forgotStatus ? authFlow.forgotStatus : ""}>
                <div className="mt-8 text-center">
                    Didn't receive an email? Please check your spam folder.
                    To receive another email, click{" "}
                    <span
                        onClick={() => setGotResponse(false)}
                        className="text-blue font-medium cursor-pointer"
                    >
                        here
                    </span>
                    .
                </div>
                <AuthButton text="Back to Login" onClick={backToLogin} />
            </AuthContainer>
        )
    } else {
        return (
            <AuthContainer title="Please enter your email">
                    <div data-testid={AUTH_IDS.FORM}>
                        <div style={{ marginTop: 40 }}>
                            <Input
                                id={E2E_AUTH_IDS.EMAIL}
                                text="Email"
                                type="email"
                                placeholder={PLACEHOLDER.EMAIL}
                                onChange={changeEmail}
                                onKeyPress={onKeyPress}
                                value={email}
                                valid={checkEmail(email)}
                            />
                        </div>
                    </div>
                    <div data-testid={AUTH_IDS.BUTTON}>
                        <button
                            className="rounded bg-blue dark:bg-mint px-8 py-3 mt-4 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
                            style={{
                                opacity: checkEmail(email) ? 1.0 : 0.6,
                            }}
                            onClick={forgot}
                            disabled={!checkEmail(email)}
                        >
                            Reset
                        </button>
                    </div>
                    <div data-testid={AUTH_IDS.SWITCH}>
                        <div className="mt-7">
                            <AuthNavigator
                                beforeLink="Remember your password?"
                                link="Log in"
                                afterLink=" instead."
                                redirect={fractalRoute(routeMap.AUTH)}
                            />
                        </div>
                    </div>
            </AuthContainer>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: {
        authFlow: AuthFlow
    }
}) => {
    return {
        authFlow: state.AuthReducer.authFlow,
    }
}

export default connect(mapStateToProps)(Forgot)
