// npm imports
import React, { useState, useEffect } from "react"

// Component imports
import AuthContainer from "pages/auth/shared/components/authContainer"

// Function imports
import { useFractalProvider } from "pages/auth/pages/forgot/shared/store/store"
import { routeMap, fractalRoute } from "shared/constants/routes"
import history from "shared/utils/history"
import AuthNavigator from "pages/auth/shared/components/authNavigator"

// Constant + type imports
import { FractalHTTPCode } from "@fractal/core-ts/types/api"
import { EmailStatus } from "pages/auth/pages/forgot/pages/emailForm/shared/constants"

const FormSubmitted = () => {
    /*
        Component for user to provide their email to receive a password
        reset email
    */

    const { state } = useFractalProvider()
    const [ title, setTitle ] = useState("")
    const [ text, setText ] = useState("")

    useEffect(() => {
        if(state && !state.emailServerResponse) {
            history.push(fractalRoute(routeMap.AUTH.FORGOT))
        } else if (state.emailServerResponse) {
            switch(state.emailServerResponse) {
                case FractalHTTPCode.SUCCESS:
                    setTitle(EmailStatus.SUCCESS.title)
                    setText(EmailStatus.SUCCESS.text)
                    break
                case FractalHTTPCode.UNAUTHORIZED:
                    setTitle(EmailStatus.UNAUTHORIZED.title)
                    setText(EmailStatus.UNAUTHORIZED.text)
                    break
                default:
                    setTitle(EmailStatus.FAILURE.title)
                    setText(EmailStatus.FAILURE.text)
                    break
            }
        }
    }, [state])

    return (
        <AuthContainer title={title}>
            <div className="text-gray mt-4 text-center">
                {text}
            </div>
            <AuthNavigator
                beforeLink="To receive another email, click "
                link="here"
                afterLink="."
                redirect={fractalRoute(routeMap.AUTH.FORGOT.EMAIL)}
            />
        </AuthContainer>
    )
}

export default FormSubmitted
