import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { FormControl } from "react-bootstrap"

import "styles/auth.css"

import { GoogleLogin } from "react-google-login"
import googleIcon from "assets/icons/google-login.svg"

const LoginView = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const [mode, setMode] = useState("login")

    const responseGoogleSuccess = (res: any) => {
        // the only real difference between the two is the error handling/set failure
        // if (props.mode === SIGN_UP) {
        //     props.dispatch(googleSignup(res.code))
        // } else if (props.mode === LOG_IN) {
        //     props.dispatch(googleLogin(res.code))
        // }

        // props.dispatch(setLoading(GOOGLE_BOX, true))

        console.log("google success")
    }

    const responseGoogleFailure = (res: any) => {
        console.log(`response google failure`)
    }

    const googleButton = (renderProps: any) => (
        <button
            onClick={renderProps.onClick}
            disabled={renderProps.disabled}
            className="action"
            style={{ padding: "20px 20px 20px 20px", marginBottom: "10px" }}
        >
            <div
                style={{
                    fontSize: width > 720 ? 20 : 16,
                    fontWeight: "bold",
                    textAlign: "center",
                    marginRight: "0px",
                }}
            >
                {props.mode} with Google
            </div>
            <img src={googleIcon} style={{ width: 40, height: 40 }} />
        </button>
    )

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
                    style={{
                        marginTop: 15,
                    }}
                />
                <button
                    className="white-button"
                    style={{
                        width: "100%",
                        marginTop: 15,
                        background: "#93f1d9",
                        border: "none",
                    }}
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
                <GoogleLogin
                    clientId={config.keys.GOOGLE_CLIENT_ID}
                    buttonText={"Sign in with Google"}
                    accessType={"offline"}
                    responseType={"code"}
                    onSuccess={responseGoogleSuccess}
                    onFailure={responseGoogleFailure}
                    cookiePolicy={"single_host_origin"}
                    redirectUri={"postmessage"}
                    prompt={"consent"}
                    style={{ width: "100%", fontWeight: "bold" }}
                    render={(renderProps) => googleButton(renderProps)}
                />
            </div>
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(LoginView)
