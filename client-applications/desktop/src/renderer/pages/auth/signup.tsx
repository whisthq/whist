import React from "react"
import { Flipped, Flipper } from "react-flip-toolkit"

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
    signupEnabled,
    checkEmail,
    checkPassword,
    checkPasswordVerbose,
    checkConfirmPassword,
    checkConfirmPasswordVerbose,
} from "@app/utils/auth"
/* import { emailSignup } from "@app/utils/api" */

const Signup = (props: {
    loading: boolean
    warning: string
    email: string
    password: string
    confirmPassword: string
    onSignup: () => void
    onNavigate: (s: string) => void
    onChangeEmail: (s: string) => void
    onChangePassword: (s: string) => void
    onChangeConfirmPassword: (s: string) => void
}) => {
    /*
        Description:
            Component for signing up for Fractal. Contains the signup form UI and
            dispatches signup API call
        Arguments:
            onSignup((json) => void):
                Callback function fired when signup API call is sent, body of response
                is passed in as argument
    */

    const buttonState = () => {
        if (props.loading) {
            return FractalButtonState.PROCESSING
        } else {
            if (
                signupEnabled(
                    props.email,
                    props.password,
                    props.confirmPassword
                )
            ) {
                return FractalButtonState.DEFAULT
            } else {
                return FractalButtonState.DISABLED
            }
        }
    }

    return (
        <div className="flex flex-col justify-center items-center h-screen text-center">
            <div className="w-full max-w-xs m-auto">
                <FractalFadeIn>
                    <Logo />
                    <h5 className="font-body mt-8 text-xl mb-6 font-semibold">
                        Sign up to get started
                    </h5>
                    <FractalWarning
                        type={FractalWarningType.DEFAULT}
                        warning={props.warning}
                        className="mt-4"
                    />
                    <Flipper
                        flipKey={checkPassword(props.password).toString()}
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
                                        onChange={props.onChangeEmail}
                                        onEnterKey={props.onSignup}
                                        value={props.email}
                                        state={
                                            checkEmail(props.email)
                                                ? FractalInputState.SUCCESS
                                                : FractalInputState.DEFAULT
                                        }
                                        className="mt-1"
                                    />
                                </div>
                                <FractalWarning
                                    type={FractalWarningType.SMALL}
                                    warning={checkPasswordVerbose(
                                        props.password
                                    )}
                                    className="mt-3 float-right font-semibold"
                                />
                                <div key="password">
                                    <h5 className="font-body text-left font-semibold mt-4 text-sm">
                                        Password
                                    </h5>
                                    <FractalInput
                                        type="password"
                                        placeholder="Password"
                                        onChange={props.onChangePassword}
                                        onEnterKey={props.onSignup}
                                        value={props.password}
                                        state={
                                            checkPassword(props.password)
                                                ? FractalInputState.SUCCESS
                                                : FractalInputState.DEFAULT
                                        }
                                        className="mt-1"
                                    />
                                </div>
                                <FractalWarning
                                    type={FractalWarningType.SMALL}
                                    warning={checkConfirmPasswordVerbose(
                                        props.password,
                                        props.confirmPassword
                                    )}
                                    className="mt-3 float-right font-semibold"
                                />
                                <div key="confirm-password">
                                    {checkPassword(props.password) && (
                                        <>
                                            <h5 className="font-body text-left font-semibold mt-4 text-sm">
                                                Confirm Password
                                            </h5>
                                            <FractalInput
                                                type="password"
                                                placeholder="Password"
                                                onChange={
                                                    props.onChangeConfirmPassword
                                                }
                                                onEnterKey={props.onSignup}
                                                value={props.confirmPassword}
                                                state={
                                                    checkConfirmPassword(
                                                        props.password,
                                                        props.confirmPassword
                                                    )
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
                        onClick={props.onSignup}
                    />
                    <FractalNavigation
                        url="/login"
                        text="Already have an account? Log in here."
                        linkText="here"
                        className="relative top-4"
                        onClick={props.onNavigate}
                    />
                </FractalFadeIn>
            </div>
        </div>
    )
}

export default Signup
