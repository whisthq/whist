/* eslint-disable no-console */
import React, { Dispatch, useContext } from "react"
import { connect } from "react-redux"
import { GoogleLogin } from "react-google-login"
import { FaGoogle } from "react-icons/fa"

import MainContext from "shared/context/mainContext"
import { config } from "shared/constants/config"
import { ScreenSize } from "shared/constants/screenSizes"

import styles from "styles/auth.module.css"

const GoogleButton = (props: {
    dispatch: Dispatch<any>
    login: (code: string) => void
}) => {
    const { width } = useContext(MainContext)

    const responseGoogleSuccess = (res: any) => {
        props.login(res.code)
        //TODO might want to remove this and use the warnings in auth? https://github.com/fractal/website/issues/340
    }

    const responseGoogleFailure = (res: any) => {
        //TODO might want to remove this and use the warnings in auth? https://github.com/fractal/website/issues/340
        // props.dispatch(
        //     updateAuthFlow({
        //         loginWarning: "Google response failure",
        //         signupWarning: "Google response failure",
        //     })
        // )
    }

    const googleButton = (renderProps: any) => (
        <button
            onClick={renderProps.onClick}
            disabled={renderProps.disabled}
            className={styles.googleButton}
        >
            <FaGoogle
                style={{
                    color: "white",
                    position: "relative",
                    top: 5,
                    marginRight: 15,
                }}
            />
            <div
                style={{
                    fontSize: width > ScreenSize.MEDIUM ? 16 : 14,
                }}
            >
                Log in with Google
            </div>
        </button>
    )

    let GOOGLE_CLIENT_ID: string = ""
    if (config.keys.GOOGLE_CLIENT_ID == null) {
        console.error("Error: environment variable GOOGLE_CLIENT_ID not set")
    } else {
        GOOGLE_CLIENT_ID = config.keys.GOOGLE_CLIENT_ID as string
    }

    return (
        <div>
            <GoogleLogin
                clientId={GOOGLE_CLIENT_ID}
                buttonText={"Sign in with Google"}
                accessType={"offline"}
                responseType={"code"}
                onSuccess={responseGoogleSuccess}
                onFailure={responseGoogleFailure}
                cookiePolicy={"single_host_origin"}
                redirectUri={"postmessage"}
                prompt={"consent"}
                style={{ width: "100%" }}
                render={(renderProps) => googleButton(renderProps)}
            />
        </div>
    )
}

const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(GoogleButton)
