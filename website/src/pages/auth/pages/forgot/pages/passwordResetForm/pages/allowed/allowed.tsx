/* eslint-disable no-console */

// npm imports
import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"
import { useLocation } from "react-router-dom"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthNavigator from "pages/auth/shared/components/authNavigator"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"

// Function imports
import { validateAccessToken } from "shared/api/index"
import {
    checkPassword,
    checkPasswordVerbose,
    checkConfirmPasswordVerbose
} from "pages/auth/shared/helpers/authHelpers"
import history from "shared/utils/history"
import {routeMap, fractalRoute} from "shared/constants/routes"

// Constant + type imports
import FractalKey from "shared/types/input"
import { resetPassword } from "store/actions/auth/sideEffects"
import { RESET_IDS } from "testing/utils/testIDs"

const Allowed = (props: {
    accessToken: string,
    dispatch: Dispatch<any>
}) => {
    const { dispatch, accessToken } = props

    // Keep track of the password that user types
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    // Dynamic warnings if password is not valid (e.g. too short, doesn't match)
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    // Toggles when to show loading animation
    const [processing, setProcessing] = useState(true)
    // For access token in URL, which determines if user is allowed to reset their password
    const [accessTokenValid, setAccessTokenValid] = useState(false)

    // Dispatches password reset API call
    const reset = () => {
        if (checkPassword(password) && (confirmPassword === password)) {
            dispatch(
                resetPassword(password)
            )
        }
    }

    // Handles key strokes
    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    // Handles ENTER key press
    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            reset()
        }
    }

    // On page load, read the access token from the URL and validate it to determine
    // if the user is allowed to reset their password
    useEffect(() => {
        if(accessToken) {
            validateAccessToken(accessToken).then(({json}) => {
                const isValid = (json && json.status === 200)
                setAccessTokenValid(isValid)
                setProcessing(false)
            })
        } else {
            setProcessing(false)
        }
    }, [])

    // On password change, display warnings if password is invalid
    useEffect(() => {
        setPasswordWarning(checkPasswordVerbose(password))
    }, [password])

    // On confirm password change, display warnings if confirm password is invalid
    useEffect(() => {
        const warning = checkConfirmPasswordVerbose(password, confirmPassword)
        if(warning !== confirmPasswordWarning) {
            setConfirmPasswordWarning(warning)
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [confirmPassword])

    return (
        <AuthContainer title="Please enter a new password">
                <div data-testid={RESET_IDS.FORM}>
                    <PasswordConfirmForm
                        changePassword={changePassword}
                        changeConfirmPassword={changeConfirmPassword}
                        onKeyPress={onKeyPress}
                        password={password}
                        confirmPassword={confirmPassword}
                        passwordWarning={passwordWarning}
                        confirmPasswordWarning={confirmPasswordWarning}
                        isFirstElement={true}
                    />
                </div>
                <div data-testid={RESET_IDS.BUTTON}>
                    <button
                        className="rounded bg-blue dark:bg-mint px-8 py-3 mt-4 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
                        onClick={reset}
                    >
                        Reset
                    </button>
                </div>
                <div className="mt-7">
                    <AuthNavigator
                        beforeLink="Remember your password?"
                        link="Log in"
                        afterLink=" instead."
                        redirect={fractalRoute(routeMap.AUTH)}
                    />
                </div>
        </AuthContainer>
    )
}

export default connect()(Allowed)
