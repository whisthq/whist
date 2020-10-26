import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { validateVerificationToken } from "store/actions/auth/sideEffects"

import "styles/auth.css"

/**
 * A title component that announces the state of the Verify page when it is not processing. It is used
 * to say (specifically) when verify is not being arrived at via email or when verification failed.
 * @param props.title the text announcing what state we are at.
 * @param props.subtitle a small subtitle than can add some clarification.
 */
const Title = (props: { title: string; subtitle?: string }) => {
    const { title, subtitle } = props

    return (
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
                    {title}
                </h2>
            </div>
            {subtitle ? (
                <div
                    style={{
                        color: "#3930b8", // let's add some variety
                        textAlign: "center",
                    }}
                >
                    {subtitle}
                </div>
            ) : (
                <div />
            )}
        </div>
    )
}

const VerifyView = (props: {
    dispatch: any
    user: any
    authFlow: any
    token: any
    validToken: boolean
}) => {
    const { dispatch, user, authFlow, token, validToken } = props

    // visual state constants
    const [processing, setProcessing] = useState(false)

    // logic
    const validUser = user.user_id && user.user_id !== ""

    useEffect(() => {
        if (validUser && validToken && !processing) {
            dispatch(validateVerificationToken(token))

            setProcessing(true)
        }
        // want onComponentMount basically (thus [] ~ no deps ~ called only at the very beginning)
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [])

    useEffect(() => {
        if (user.emailVerified) {
            setProcessing(false)
        }
    }, [user.emailVerified, authFlow.verificationAttemptsExecuted])

    if (!validToken) {
        return <Title title="Check your inbox" subtitle="(and/or spam)" />
    } else {
        if (processing) {
            // Conditionally render the loading screen as we wait for signup API call to return
            return (
                <div>
                    <PuffAnimation />
                </div>
            )
        } else {
            // this state is reached after processing has finished and failed
            return <Title title="Failed to verify" />
        }
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(VerifyView)
