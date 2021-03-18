import React, { useState } from "react"
import { Flipper, Flipped } from "react-flip-toolkit"
import FadeIn from "react-fade-in"

import { Logo } from "@app/renderer/pages/auth/shared/components/logo"
import { FractalInput, FractalInputState } from "@app/components/html/input"
import { AuthWarning } from "@app/components/custom/warning"
import { FractalButton, FractalButtonState } from "@app/components/html/button"
import { FractalNavigation } from "@app/components/custom/navigation"

import {
    loginEnabled,
    checkEmail,
    checkPassword,
} from "@app/renderer/pages/auth/shared/helpers/authHelpers"

const Login = (props: { onSignup: (json: object) => void }) => {
    const { onSignup } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [confirmPassword, setConfirmPassword] = useState("")
    const [processing, setProcessing] = useState(false)
    const [signupWarning, setSignupWarning] = useState("")

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

    const signup = () => {
        // TODO: Signup API call
    }

    return (
        <div className="flex flex-col justify-center items-center bg-white h-screen text-center">
            <div className="w-full max-w-xs m-auto">
                <FadeIn>
                    <Logo />
                    <h5 className="font-body mt-8 text-xl mb-6 font-semibold">
                        Sign up to get started
                    </h5>
                    <AuthWarning warning={signupWarning} />
                    <Flipper
                        flipKey={checkPassword(password).toString()}
                        spring="stiff"
                    >
                        <Flipped flipId="form">
                            <ul className="listContents">
                                <div key="email">
                                    <h5 className="font-body text-left font-semibold mt-4 text-sm">
                                        Email
                                    </h5>
                                    <FractalInput
                                        type="email"
                                        placeholder="Email"
                                        onChange={(email: string) =>
                                            setEmail(email)
                                        }
                                        onEnterKey={signup}
                                        value={email}
                                        state={
                                            checkEmail(email)
                                                ? FractalInputState.SUCCESS
                                                : FractalInputState.DEFAULT
                                        }
                                        className="mt-1"
                                    />
                                </div>
                                <div key="password">
                                    <h5 className="font-body text-left font-semibold mt-4 text-sm">
                                        Password
                                    </h5>
                                    <FractalInput
                                        type="password"
                                        placeholder="Password"
                                        onChange={(password: string) =>
                                            setPassword(password)
                                        }
                                        onEnterKey={signup}
                                        value={password}
                                        state={
                                            checkPassword(password)
                                                ? FractalInputState.SUCCESS
                                                : FractalInputState.DEFAULT
                                        }
                                        className="mt-1"
                                    />
                                </div>
                                <div key="confirm-password">
                                    {checkPassword(password) && (
                                        <>
                                            <h5 className="font-body text-left font-semibold mt-4 text-sm">
                                                Confirm Password
                                            </h5>
                                            <FractalInput
                                                type="password"
                                                placeholder="Password"
                                                onChange={(password: string) =>
                                                    setConfirmPassword(password)
                                                }
                                                onEnterKey={signup}
                                                value={confirmPassword}
                                                state={
                                                    checkPassword(password)
                                                        ? FractalInputState.SUCCESS
                                                        : FractalInputState.DEFAULT
                                                }
                                                className="mt-1"
                                            />
                                        </>
                                    )}
                                </div>
                            </ul>
                        </Flipped>
                    </Flipper>
                    <FractalButton
                        contents="Sign up"
                        className="mt-4 w-full"
                        state={buttonState()}
                        onClick={signup}
                    />
                    <FractalNavigation
                        url="/auth/login"
                        text="Already have an account? Log in here."
                        linkText="here"
                        className="relative top-4"
                    />
                </FadeIn>
            </div>
        </div>
    )
}

export default Login
