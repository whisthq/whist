import React from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"

import "styles/auth.css"

import Header from "shared/components/header"

const AuthCallback = (props: { callback: string }) => {
    const { callback } = props

    return (
        <div className="fractalContainer">
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

function mapStateToProps(state: { AuthReducer: { authFlow: any } }) {
    return {
        callback: state.AuthReducer.authFlow.callback,
    }
}

export default connect(mapStateToProps)(AuthCallback)
