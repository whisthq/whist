import React, { useEffect, useState } from "react"
import { connect } from "react-redux"

import LoginView from "pages/auth/views/loginView"
import SignupView from "pages/auth/views/signupView"

import * as AuthPureAction from "store/actions/auth/pure"

import "styles/auth.css"

const Auth = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { dispatch } = props
    const [mode, setMode] = useState("Log in")

    if (mode === "Log in") {
        return (
            <div>
                <LoginView />
                <div style={{ textAlign: "center", marginTop: 20 }}>
                    Need to create an account?{" "}
                    <span
                        style={{ color: "#3930b8" }}
                        className="hover"
                        onClick={() => setMode("Sign up")}
                    >
                        Sign up
                    </span>{" "}
                    here.
                </div>
            </div>
        )
    } else {
        return (
            <div>
                <SignupView />
                <div style={{ textAlign: "center", marginTop: 20 }}>
                    Already have an account?{" "}
                    <span
                        style={{ color: "#3930b8" }}
                        className="hover"
                        onClick={() => setMode("Log in")}
                    >
                        Log in
                    </span>{" "}
                    here.
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Auth)
