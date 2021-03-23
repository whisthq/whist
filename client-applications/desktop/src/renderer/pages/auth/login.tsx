import React, { useState } from "react"

import { FractalFadeIn } from "@app/components/custom/fade"
import { Logo } from "@app/components/html/logo"
import { FractalInput, FractalInputState } from "@app/components/html/input"
import {
    FractalWarning,
    FractalWarningType,
} from "@app/components/custom/warning"
import { FractalButton, FractalButtonState } from "@app/components/html/button"
import { FractalNavigation } from "@app/components/custom/navigation"

import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "@app/utils/auth"
import { emailLogin } from "@app/utils/api"
import { fractalLoginWarning } from "@app/utils/constants"

const Login = (props: { onLogin: (json: object) => void }) => {
    /*
        Description:
            Component for logging in. Contains the login form UI and 
            dispatches login API call

        Arguments:
            onLogin((json) => void): 
                Callback function fired when login API call is sent, body of response
                is passed in as argument
    */

    const { onLogin } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [processing, setProcessing] = useState(false)
    const [loginWarning, setLoginWarning] = useState("")

    const login = () => {
        if (loginEnabled(email, password)) {
            setProcessing(true)
            setLoginWarning(fractalLoginWarning.NONE)
            emailLogin(email, password).then(({ json }) => {
                if (json && json.access_token) {
                    onLogin({ ...json, email })
                } else {
                    setLoginWarning(fractalLoginWarning.INVALID)
                    setPassword("")
                }
                setProcessing(false)
            })
        } else {
            setLoginWarning(fractalLoginWarning.INVALID)
            setProcessing(false)
        }
    }

    const buttonState = () => {
        if (processing) {
            return FractalButtonState.PROCESSING
        } else {
            if (loginEnabled(email, password)) {
                return FractalButtonState.DEFAULT
            } else {
                return FractalButtonState.DISABLED
            }
        }
    }

    return (
        <div className="flex flex-col justify-center items-center bg-white h-screen text-center">
            <div className="w-full max-w-xs m-auto">
                <FractalFadeIn>
                    <Logo />
                    <h5 className="font-body mt-8 text-xl mb-6 font-semibold">
                        Log in to your account
                    </h5>
                    <FractalWarning
                        type={FractalWarningType.DEFAULT}
                        warning={loginWarning}
                        className="mt-4"
                    />
                    <h5 className="font-body text-left font-semibold mt-7 text-sm">
                        Email
                    </h5>
                    <FractalInput
                        type="email"
                        placeholder="Email"
                        onChange={(email: string) => setEmail(email)}
                        onEnterKey={login}
                        value={email}
                        state={
                            checkEmail(email)
                                ? FractalInputState.SUCCESS
                                : FractalInputState.DEFAULT
                        }
                        className="mt-1"
                    />
                    <h5 className="font-body text-left font-semibold mt-4 text-sm">
                        Password
                    </h5>
                    <FractalInput
                        type="password"
                        placeholder="Password"
                        onChange={(password: string) => setPassword(password)}
                        onEnterKey={login}
                        value={password}
                        state={
                            checkPassword(password)
                                ? FractalInputState.SUCCESS
                                : FractalInputState.DEFAULT
                        }
                        className="mt-1"
                    />
                    <FractalButton
                        contents="Log In"
                        className="mt-4 w-full"
                        state={buttonState()}
                        onClick={login}
                    />
                    <FractalNavigation
                        url="/auth/signup"
                        text="Need an account? Sign up here."
                        linkText="here"
                        className="relative top-4"
                    />
                </FractalFadeIn>
            </div>
        </div>
    )
}

export default Login
