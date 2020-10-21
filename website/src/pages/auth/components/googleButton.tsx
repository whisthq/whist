import React, { useContext } from "react"
import { connect } from "react-redux"
import { GoogleLogin } from "react-google-login"
import { FaGoogle } from "react-icons/fa"

import MainContext from "shared/context/mainContext"
import { config } from "shared/constants/config"

import "styles/auth.css"

const GoogleButton = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { width } = useContext(MainContext)

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
                    fontWeight: "bold",
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
                style={{ width: "100%", fontWeight: "bold" }}
                render={(renderProps) => googleButton(renderProps)}
            />
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(GoogleButton)
