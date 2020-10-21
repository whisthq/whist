import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router"

import PuffLoader from "react-spinners/PuffLoader"

import { validateVerificationToken } from "store/actions/auth/sideEffects"

import "styles/auth.css"

/**
 * A title component that announces the state of the Verify page when it is not processing. It is used
 * to say (specifically) when verify is not being arrived at via email or when verification failed.
 * @param text the text announcing what state we are at.
 * @param subtext a small subtitle than can add some clarification.
 */
const title = (text: string, subtext: string | null = null) => (
    <div>
        <div
            style={{
                width: 400,
                margin: "auto",
                marginTop: 120,
            }}
        >
            <h2
                style={{
                    color: "#111111",
                    textAlign: "center",
                }}
            >
                {text}
            </h2>
        </div>
        {subtext ? <div>{subtext}</div> : <div />}
    </div>
)

const Verify = (props: any) => {
    const { user, authFlow, dispatch } = props

    // token constants
    const search = useLocation().search
    const token = search.substring(1, search.length)

    // visual state constants
    const [processing, setProcessing] = useState(false)

    // logic to process the token if it exists
    const valid_user = user.user_id && user.user_id !== ""
    const valid_token = token && token.length >= 1

    // this will be called ONCE onComponentMount (since it has [])
    // this will throw a "warning" since raact get angery
    useEffect(() => {
        if (valid_user && valid_token && !processing) {
            dispatch(validateVerificationToken(user.user_id))

            setProcessing(true)
        }
    }, [])

    useEffect(() => {
        if (user.emailVerified) {
            setProcessing(false)
        }
    }, [user.emailVerified, authFlow.verificationAttemptsExecuted])

    // return visuals
    if (!valid_user || user.emailVerified) {
        return <Redirect to="/" />
    } else if (!valid_token) {
        return title("Check your inbox", "(and/or spam)")
    } else {
        if (processing) {
            // Conditionally render the loading screen as we wait for signup API call to return
            return (
                <div
                    style={{
                        width: "100vw",
                        height: "100vh",
                        position: "relative",
                    }}
                >
                    <PuffLoader
                        css="position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%);"
                        size={75}
                    />
                </div>
            )
        } else {
            // this state is reached after processing has finished and failed
            return title("Failed to verify")
        }
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        // TODO check why user is sometimes undefined (it should never be undefined)
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(Verify)
