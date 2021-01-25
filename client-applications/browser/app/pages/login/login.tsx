import React, { useState, useEffect, ChangeEvent } from "react"
import { connect } from "react-redux"

import { openExternal } from "shared/utils/general/helpers"
import { config } from "shared/constants/config"
import { Dispatch } from "shared/types/redux"
import { FractalKey } from "shared/types/input"
import { FractalRoute } from "shared/types/navigation"
import { User } from "store/reducers/auth/default"
import { validateAccessToken } from "store/actions/auth/sideEffects"
import { history } from "store/history"
import { FractalIPC } from "shared/types/ipc"
import SplashScreenComponent from "pages/login/components/splashScreen/splashScreen"
import RedirectComponent from "pages/login/components/redirect/redirect"
import GeometricComponent from "pages/login/components/geometric/geometric"

import styles from "pages/login/login.css"

export const Login = (props: {
    userID: string
    accessToken: string
    dispatch: Dispatch
}) => {
    /*
        Login page that redirects user to a browser and validates the returned from the browser
 
        Arguments:
            userID: User ID
            accessToken: Access token  
    */

    const { userID, accessToken, dispatch } = props

    const [buttonClicked, setButtonClicked] = useState(false)
    const [localAccessToken, setLocalAccessToken] = useState("")

    const ipc = require("electron").ipcRenderer
    ipc.on(FractalIPC.CUSTOM_URL, (_: any, customURL: string) => {
        if (customURL && customURL.toString().includes("fractal://")) {
            customURL = `fractal://${customURL.split("fractal://")[1]}`
            // Convert URL to URL object so it can be parsed
            const urlObj = new URL(customURL.toString())
            urlObj.protocol = "https"

            // Check to see if this is an auth request or an external app authentication
            const websiteAccessToken = urlObj.searchParams.get("accessToken")

            if (websiteAccessToken) {
                dispatch(validateAccessToken(websiteAccessToken))
            }
        }
    })

    const handleLogin = () => {
        setButtonClicked(true)
        openExternal(
            `${config.url.FRONTEND_URL}/auth/bypass?callback=fractal://auth`
        )
    }

    const changeAccessToken = (evt: ChangeEvent) => {
        evt.persist()
        setLocalAccessToken(evt.target.value)
    }

    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            dispatch(validateAccessToken(localAccessToken))
        }
    }

    useEffect(() => {
        if (userID && accessToken) {
            history.push(FractalRoute.LAUNCHER)
        }
    }, [accessToken, userID])

    return (
        <div className={styles.loginWrapper}>
            <div style={{ position: "relative" }}>
                <div style={{ position: "absolute", top: 825, left: 425 }}>
                    <GeometricComponent
                        color="#00000023"
                        scale={3}
                        flip={false}
                    />
                </div>
            </div>
            <div style={{ position: "relative" }}>
                <div style={{ position: "absolute", top: 825, right: -840 }}>
                    <GeometricComponent color="#00000023" scale={3} flip />
                </div>
            </div>
            <div className={styles.loginCenter}>
                {!buttonClicked ? (
                    <div>
                        <SplashScreenComponent onClick={handleLogin} />
                    </div>
                ) : (
                    <div>
                        <RedirectComponent
                            onClick={handleLogin}
                            onChange={changeAccessToken}
                            onKeyPress={onKeyPress}
                        />
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: { AuthReducer: { user: User } }) => {
    return {
        userID: state.AuthReducer.user.userID,
        accessToken: state.AuthReducer.user.accessToken,
    }
}

export default connect(mapStateToProps)(Login)
