import React from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"

import sharedStyles from "styles/shared.module.css"

import Header from "shared/components/header"
import { AuthFlow } from "store/reducers/auth/default"

const AuthCallback = (props: { callback: string | undefined }) => {
    /*
        Callback page for the client-app.

        The website will redirect to this page after completing the login flow for the client-app.

        Arguments:
            callback (String): the URL to "call back" to (usually "fractal://")

    */
    const { callback } = props

    return (
        <div className={sharedStyles.fractalContainer}>
            <Header dark={false} />
            <div></div>
            <div
                style={{
                    width: 500,
                    margin: "auto",
                    marginTop: 70,
                    textAlign: "center",
                }}
            >
                <h2>You will be redirected.</h2>
                <div style={{ marginTop: 25 }}>
                    Not redirected? Click{" "}
                    <a href={callback} style={{ fontWeight: "bold" }}>
                        here
                    </a>{" "}
                    to try again, or click <Link to="/dashboard">here</Link> to
                    return home.
                </div>
            </div>
        </div>
    )
}

const mapStateToProps = (state: { AuthReducer: { authFlow: AuthFlow } }) => {
    return {
        callback: state.AuthReducer.authFlow.callback,
    }
}

export default connect(mapStateToProps)(AuthCallback)
