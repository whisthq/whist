/* eslint-disable no-console */
import React, { Dispatch, useEffect, useState } from "react"
import { connect } from "react-redux"

import { PuffAnimation } from "shared/components/loadingAnimations"
import {
    validateVerificationToken,
    sendVerificationEmail,
} from "store/actions/auth/sideEffects"
import { updateAuthFlow, resetUser } from "store/actions/auth/pure"
import history from "shared/utils/history"
import { VERIFY_IDS } from "testing/utils/testIDs"

import AuthContainer from "pages/auth/components/authContainer"
import { User, AuthFlow } from "shared/types/reducers"


export const RetryButton = (props: {
    text: string
    canClick: boolean
    onClick: (evt: any) => any
}) => (
    /*
        Component button for retrying the email send.

        Arguments:
            text (string): text that the button should display
            canClick (boolean): boolean that represents whether the button
                should be clicked or not.
            onClick (method): function to be called when the button is clicked
    */

    <button
        className="rounded bg-blue dark:bg-mint px-8 py-3 mt-8 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
        style={{
<<<<<<< HEAD
            marginTop: 40,
            opacity: props.canClick ? 1.0 : 0.6,
=======
            opacity: props.checkEmail ? 1.0 : 0.6,
>>>>>>> cd0c3f2 (refactored all ui and some of logic)
        }}
        onClick={props.onClick}
        disabled={!props.canClick}
    >
        {props.text}
    </button>
)

const VerifyView = (props: {
    dispatch: Dispatch<any>
    user: User
    authFlow: AuthFlow
    token: string
    validToken: boolean
}) => {
    /*
        Component for verifying user emails

        The user will be redirected here from a link in the verification email
        that we send. Sends a verification email on render, and also contains a
        button to send more if the user doesn't receive the first.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
            token (string): the token passed into the url (i.e. fractal.co/verify?{token})
            validToken (boolean): whether the token exists or not
    */
    const { dispatch, user, authFlow, token, validToken } = props

    // visual state constants
    const [processing, setProcessing] = useState(false)
    const [canRetry, setCanRetry] = useState(true)
    const [sentRetry, setSentRetry] = useState(false)

    // logic
    const validUser =
        user.userID &&
        user.userID !== "" &&
        user.accessToken &&
        user.accessToken !== ""
            ? true
            : false

    // used for button
    const retryMessage = validUser
        ? sentRetry
            ? "Sent!"
            : canRetry
            ? "Send Again"
            : "Sending..."
        : "Login to Re-send"

    const reset = () => {
        dispatch(resetUser())
        history.push("/auth")
    }

    const sendWithDelay = (evt: any) => {
        // this can only be reached when there is a valid user
        setCanRetry(false)

        dispatch(
            updateAuthFlow({
                verificationEmailsSent: authFlow.verificationEmailsSent
                    ? authFlow.verificationEmailsSent + 1
                    : 1,
            })
        )

        let userID: string | undefined
        if (user.userID == null) {
            console.error("Error: no userId")
        } else {
            userID = user.userID as string
        }

        let name: string | undefined
        if (user.name == null) {
            console.error("Error: no name")
        } else {
            name = user.name as string
        }

        let emailVerificationToken: string | undefined
        if (user.emailVerificationToken == null) {
            console.error("Error: no emailVerificationToken")
        } else {
            emailVerificationToken = user.emailVerificationToken as string
        }

        dispatch(sendVerificationEmail(userID, name, emailVerificationToken))
        setTimeout(() => {
            // first show them that it's been sent
            setSentRetry(true)
            setTimeout(() => {
                // then return to the original state
                setCanRetry(true)
                setSentRetry(false)
            }, 3000)
        }, 2000)
    }

    useEffect(() => {
        if (validUser && validToken && !processing) {
            dispatch(validateVerificationToken(token))
        }
        setProcessing(true)
        // want onComponentMount basically (thus [] ~ no deps ~ called only at the very beginning)
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    useEffect(() => {
        if (
            authFlow.verificationStatus === "success" ||
            authFlow.verificationStatus === "failed"
        ) {
            setProcessing(false)
        }
    }, [authFlow.verificationStatus])

    if (!validToken) {
        return (
            <AuthContainer title="Check your email for a verification email">
                <RetryButton
                    text={retryMessage}
                    checkEmail={validUser && canRetry}
                    onClick={sendWithDelay}
                />
                <button
                    className="w-full mt-6 text-small duration-500 text-gray"
                    onClick={reset}
                >
                    Back to Login
                </button>
            </AuthContainer>
        )
    } else {
        if (processing) {
            // Conditionally render the loading screen as we wait for signup API call to return
            return (
                <AuthContainer title="Please wait, verifying your email">
                    <div data-testid={VERIFY_IDS.LOAD}>
                        <PuffAnimation />
                    </div>
                </AuthContainer>
            )
        } else {
            // this state is reached after processing has finished and failed
            return (
                <AuthContainer>
                    <>
                        {authFlow.verificationStatus === "success" && (
                            <div data-testid={VERIFY_IDS.SUCCESS}>
                                <div className="text-3xl font-medium text-center mt-12">Success! Redirecting you.</div>
                                <PuffAnimation />
                            </div>
                        )}
                    </>
                    <>
                        {authFlow.verificationStatus === "failed" && (
                            <div data-testid={VERIFY_IDS.FAIL}>
                                <div className="text-3xl font-medium text-center mt-12">An error occured verifying your email.</div>
                                <div data-testid={VERIFY_IDS.RETRY}>
                                    <RetryButton
                                        text={retryMessage}
                                        checkEmail={validUser && canRetry}
                                        onClick={sendWithDelay}
                                    />
                                </div>
                            </div>
                        )}
                    </>
                </AuthContainer>
            )
        }
    }
}

const mapStateToProps = (state: {
    AuthReducer: {
        user: User
        authFlow: AuthFlow
    }
}) => {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(VerifyView)
