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

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthButton from "pages/auth/shared/components/authButton"
import AuthNavigator from "pages/auth/shared/components/authNavigator"
import PasswordConfirmForm from "shared/components/passwordConfirmForm"

// Function imports
import {
    checkPassword,
    checkPasswordVerbose,
    checkConfirmPasswordVerbose
} from "pages/auth/shared/helpers/authHelpers"
import {routeMap, fractalRoute} from "shared/constants/routes"
import { useFractalProvider } from "pages/auth/pages/forgot/shared/store/store"
import history from "shared/utils/history"

// Constant + type imports
import FractalKey from "shared/types/input"
import { resetPassword } from "shared/api/index"
import { RESET_IDS } from "testing/utils/testIDs"
import { FractalHTTPCode } from "@fractal/core-ts/types/api"

const Allowed = (props: {
    dispatch: Dispatch<any>
}) => {
    const { state } = useFractalProvider()
    
    // Keep track of the password that user types
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    // Dynamic warnings if password is not valid (e.g. too short, doesn't match)
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    // Display loading animation
    const [processing, setProcessing] = useState(false)
    
    // Dispatches password reset API call
    const reset = () => {
        // TODO: Call reset password with the email
        if (checkPassword(password) && (confirmPassword === password)) {
            setProcessing(true)
            resetPassword(state.accessToken, state.email, password).then(({ json, response }) => {
                if(json && response.status === FractalHTTPCode.SUCCESS) {
                    history.push(fractalRoute(routeMap.AUTH.FORGOT.RESET.SUCCESS))
                } else {
                    history.push(fractalRoute(routeMap.AUTH.FORGOT.RESET.ERROR))
                }
                setProcessing(false)
            })
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
            <AuthButton
                text="Reset"
                onClick={reset}
                processing={processing}
            />
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
