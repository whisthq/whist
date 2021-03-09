import React from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"

import sharedStyles from "styles/shared.module.css"

import Header from "shared/components/header"
import { User, AuthFlow } from "shared/types/reducers"

const AuthCallback = (props: { authFlow: AuthFlow; user: User }) => {
    /*
        Callback page for the client-app.

        The website will redirect to this page after completing the login flow for the client-app.

        Arguments:
            user (User): User from Redux state
            authFlow (AuthFlow): AuthFlow from Redux state

    */
    const { authFlow, user } = props
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
                    <a href={authFlow.callback} style={{ fontWeight: "bold" }}>
                        here
                    </a>{" "}
                    to try again, or click <Link to="/dashboard">here</Link> to
                    return home.
                </div>
                <p id="encryptionToken" hidden>
                    {user.encryptionToken}
                </p>
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { authFlow: AuthFlow; user: User }
}) => {
    return {
        authFlow: state.AuthReducer.authFlow,
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(AuthCallback)
