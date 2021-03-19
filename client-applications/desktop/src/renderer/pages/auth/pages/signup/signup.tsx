import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
} from "react"

import { Logo } from "@app/components/custom/logo"
import { FractalInput, FractalInputState } from "@app/components/html/input"
import { AuthWarning } from "@app/components/custom/warning"
import { FractalButton, FractalButtonState } from "@app/components/html/button"
import {
    checkPasswordVerbose,
    checkEmailVerbose,
    signupEnabled,
    checkEmail,
} from "@app/renderer/pages/auth/shared/helpers/authHelpers"
import { emailSignup } from "@app/renderer/pages/auth/pages/signup/shared/utils/api"
import { fractalSignupWarning } from "@app/renderer/pages/auth/pages/signup/shared/utils/constants"

import PasswordConfirmForm from "shared/components/passwordConfirmForm"
import FractalKey from "shared/types/input"

const Signup = (props: {
    onSignup: (json: object) => void
}) => {
    /*
        Component for signing up for Fractal
        Contains the form to signup, and also dispatches an API request to
        the server to authenticate the user.
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state
    */
    const { onSignup } = props
    const [email, setEmail] = useState("")
    const [name, setName] = useState("")
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [emailWarning, setEmailWarning] = useState("")
    const [nameWarning, setNameWarning] = useState("")
    const [passwordWarning, setPasswordWarning] = useState("")
    const [confirmPasswordWarning, setConfirmPasswordWarning] = useState("")
    const [enteredName, setEnteredName] = useState(false)

    const [processing, setProcessing] = useState(false)
    const [signupWarning, setSignupWarning] = useState("")

    // Dispatches signup API call
    const signup = () => {
        if (signupEnabled(email, name, password, confirmPassword)) {
            setProcessing(true)
            emailSignup(email, password, name, "").then(({ json, response }) => {
                if (json && response.statue === 200){
                    setSignupWarning(fractalSignupWarning.NONE)
                    onSignup(json)
                } else {
                    setSignupWarning(fractalSignupWarning.ACCOUNT_EXISTS)
                    setEmail("")
                    setPassword("")
                    setName("")
                    setConfirmPassword("")
                }
                setProcessing(false)
            })
        } else {
            setProcessing(false)
        }
    }

    // Handles ENTER key press
    const onKeyPress = (evt: KeyboardEvent) => {
        if (
            evt.key === FractalKey.ENTER
        ) {
           signup
        }
    }

    // Updates email, password, confirm password fields as user types

    const changePassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setPassword(evt.target.value)
    }

    const changeConfirmPassword = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setConfirmPassword(evt.target.value)
    }

    const checkNameVerbose = (n: string): string => {
        if (n.length > 0) {
            return ""
        } else {
            return "Required"
        }
    }

    const checkName = (n: string): boolean => {
        return n.length > 0
    }

    // Removes loading screen on prop change and page load
    useEffect(() => {
        setProcessing(false)
    }, [user.userID, authFlow])

    // Email and password dynamic warnings
    useEffect(() => {
        setEmailWarning(checkEmailVerbose(email))
    }, [email])

    useEffect(() => {
        setPasswordWarning(checkPasswordVerbose(password))
    }, [password])

    useEffect(() => {
        if (enteredName) {
            setNameWarning(checkNameVerbose(name))
        } else {
            setNameWarning("")
        }
    }, [name, enteredName])

    useEffect(() => {
        if (
            confirmPassword !== password &&
            password.length > 0 &&
            confirmPassword.length > 0
        ) {
            setConfirmPasswordWarning("Doesn't match")
        } else {
            setConfirmPasswordWarning("")
        }
        // we only want to change on a specific state change
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [confirmPassword])

    const buttonState = () => {
        if (processing) {
            return FractalButtonState.PROCESSING
        } else {
            if (signupEnabled(email, name, password, confirmPassword)) {
                return FractalButtonState.DEFAULT
            } else {
                return FractalButtonState.DISABLED
            }
        }
    }
        return (
                <div className="flex flex-col justify-center items-center bg-white h-screen text-center">
                    <div className="w-full max-w-xs m-auto">
                    <Logo />
                    <h5 className="font-body mt-8 text-xl mb-6 font-semibold">
                    Let's get started.
                    </h5>
                    <AuthWarning warning={signupWarning} />
                    <h5 className="font-body text-left font-semibold mt-7 text-sm">
                    Email
                    </h5>
                    <FractalInput
                    type="email"
                    placeholder="Email"
                    onChange={(email: string) => setEmail(email)}
                    onEnterKey={signup}
                    value={email}
                    state={
                        checkEmail(email)
                            ? FractalInputState.SUCCESS
                            : FractalInputState.DEFAULT
                    }
                    className="mt-1"
                    />

                    <h5 className="font-body text-left font-semibold mt-7 text-sm">
                    Name
                    </h5>
                    <FractalInput
                    type="name"
                    placeholder="Name"
                    onChange={(name: string) => setName(name)}
                    onEnterKey={signup}
                    value={name}
                    state={
                        checkName(name)
                            ? FractalInputState.SUCCESS
                            : FractalInputState.DEFAULT
                    }
                    className="mt-1"
                    />

                        <PasswordConfirmForm
                            changePassword={changePassword}
                            changeConfirmPassword={changeConfirmPassword}
                            onKeyPress={onKeyPress}
                            password={password}
                            confirmPassword={confirmPassword}
                            passwordWarning={passwordWarning}
                            confirmPasswordWarning={confirmPasswordWarning}
                            isFirstElement={false}
                        />
                        <FractalButton
                            contents="Sign Up"
                            className="mt-4 w-full"
                            state={buttonState()}
                            onClick={signup}
                        />

                    </div>
                </div>
        )
    
}

export default Signup