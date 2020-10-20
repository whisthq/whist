import React, { useState, useContext } from "react"
import { connect } from "react-redux"
import { FormControl } from "react-bootstrap"

import "styles/auth.css"

import MainContext from "shared/context/mainContext"
import * as AuthSideEffect from "store/actions/auth/sideEffects"

import GoogleButton from "pages/auth/components/googleButton"

const LoginView = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { dispatch } = props

    const [email, setEmail] = useState("")
    const [password, setPassword] = useState("")
    const [processing, setProcessing] = useState(false)

    const onKeyPress = (evt: any) => {
        if (
            evt.key === "Enter" &&
            email.length > 4 &&
            password.length > 6 &&
            email.includes("@")
        ) {
            setProcessing(true)
            dispatch(AuthSideEffect.emailLogin(email, password))
        }
    }

    const login = () => {
        if (email.length > 4 && password.length > 6 && email.includes("@")) {
            console.log("dispatching")
            setProcessing(true)
            dispatch(AuthSideEffect.emailLogin(email, password))
        }
    }

    const changeEmail = (evt: any): any => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const changePassword = (evt: any): any => {
        evt.persist()
        setPassword(evt.target.value)
    }

    if (processing) {
        return <div>Processing</div>
    } else {
        return (
            <div>
                <div
                    style={{
                        width: 400,
                        margin: "auto",
                        marginTop: 150,
                    }}
                >
                    <h2
                        style={{
                            color: "#111111",
                            textAlign: "center",
                        }}
                    >
                        Welcome back!
                    </h2>
                    <FormControl
                        type="email"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Email Address"
                        className="input-form"
                        onChange={changeEmail}
                        onKeyPress={onKeyPress}
                        value={email}
                        style={{
                            marginTop: 40,
                        }}
                    />
                    <FormControl
                        type="password"
                        aria-label="Default"
                        aria-describedby="inputGroup-sizing-default"
                        placeholder="Password"
                        className="input-form"
                        onChange={changePassword}
                        onKeyPress={onKeyPress}
                        value={password}
                        style={{
                            marginTop: 15,
                        }}
                    />
                    <button
                        className="white-button"
                        style={{
                            width: "100%",
                            marginTop: 15,
                            background: "#3930b8",
                            border: "none",
                            color: "white",
                            fontSize: 16,
                            paddingTop: 20,
                            paddingBottom: 20,
                        }}
                        onClick={login}
                    >
                        Log In
                    </button>
                    <div
                        style={{
                            height: 1,
                            width: "100%",
                            marginTop: 30,
                            marginBottom: 30,
                            background: "#EFEFEF",
                        }}
                    ></div>
                    <GoogleButton />
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

export default connect(mapStateToProps)(LoginView)
