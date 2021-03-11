// npm imports
import React, { useState, KeyboardEvent, ChangeEvent } from "react"

// Component imports
import AuthNavigator from "pages/auth/shared/components/authNavigator"
import AuthContainer from "pages/auth/shared/components/authContainer"
import AuthLoader from "pages/auth/shared/components/authLoader"

// Function imports
import { useFractalProvider } from "pages/auth/pages/forgot/shared/store/store"
import { forgotPassword } from "shared/api/index"
import { checkEmail } from "pages/auth/shared/helpers/authHelpers"
import { AUTH_IDS, E2E_AUTH_IDS } from "testing/utils/testIDs"
import { routeMap, fractalRoute } from "shared/constants/routes"
import history from "shared/utils/history"

// Constant + type imports
import { FractalHTTPCode } from "@fractal/core-ts/types/api"
import Input from "shared/components/input"
import FractalKey from "shared/types/input"
import PLACEHOLDER from "shared/constants/form"

const Form = () => {
    /*
        Component for user to provide their email to receive a password
        reset email
    */
    const { dispatch } = useFractalProvider()

    const [email, setEmail] = useState("")
    const [processing, setProcessing] = useState(false)

    const loadingTitle = "Verifying your email"

    const forgot = () => {
        if (checkEmail(email)) {
            setProcessing(true)
            forgotPassword(email).then(({ json, success }) => {
                if (json && json.verified && success) {
                    // Set password reset email and redirect
                    dispatch({
                        body: {
                            email: email,
                            emailServerResponse: FractalHTTPCode.SUCCESS,
                        },
                    })
                } else {
                    if (!json.verified) {
                        dispatch({
                            body: {
                                emailServerResponse: FractalHTTPCode.UNAUTHORIZED
                            }
                        })
                    } else {
                        dispatch({
                            body: {
                                emailServerResponse:
                                    FractalHTTPCode.BAD_REQUEST,
                            },
                        })                    
                    }
                }
                setProcessing(false)
                history.push(fractalRoute(routeMap.AUTH.FORGOT.EMAIL.SUBMITTED))
            })
        }
    }

    const changeEmail = (evt: ChangeEvent<HTMLInputElement>) => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            forgot()
        }
    }

    if (processing) {
        return(
            <>
                <AuthLoader title={loadingTitle} />
            </>
        )
    } else {
        return (
            <AuthContainer title="Please enter your email">
                <div data-testid={AUTH_IDS.FORM}>
                    <div className="mt-7">
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
}

export default Form
