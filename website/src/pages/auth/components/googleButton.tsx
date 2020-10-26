import React, { useContext } from "react"
import { connect } from "react-redux"
import { GoogleLogin } from "react-google-login"
import { FaGoogle } from "react-icons/fa"

import MainContext from "shared/context/mainContext"
import { config } from "shared/constants/config"

import "styles/auth.css"
import { updateAuthFlow } from "store/actions/auth/pure"

const GoogleButton = (props: { dispatch: any; login: (code: any) => any }) => {
    const { width } = useContext(MainContext)

    const responseGoogleSuccess = (res: any) => {
        props.login(res.code)
        //TODO might want to remove this and use the warnings in auth?
        props.dispatch(
            updateAuthFlow({
                loginWarning: "",
                signupWarning: "",
            })
        )
    }

    const responseGoogleFailure = (res: any) => {
        //TODO might want to remove this and use the warnings in auth?
        props.dispatch(
            updateAuthFlow({
                loginWarning: "Google response failure",
                signupWarning: "Google response failure",
            })
        )
    }

    const googleButton = (renderProps: any) => (
        <button
            onClick={renderProps.onClick}
            disabled={renderProps.disabled}
            className="google-button"
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
                    fontSize: width > 720 ? 16 : 14,
                }}
            >
                Log in with Google
            </div>
        </button>
    )

    return (
        <div>
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
                style={{ width: "100%" }}
                render={(renderProps) => googleButton(renderProps)}
            />
        </div>
    )
}

function mapStateToProps(state: any) {
    return {}
}

export default connect(mapStateToProps)(GoogleButton)
