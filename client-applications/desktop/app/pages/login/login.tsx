import React, { useState, useEffect, ChangeEvent } from "react"
import { connect } from "react-redux"
import { useMutation, useSubscription } from "@apollo/client"

import { openExternal } from "shared/utils/general/helpers"
import { config } from "shared/constants/config"
import { Dispatch } from "shared/types/redux"
import { FractalKey } from "shared/types/input"
import { FractalRoute } from "shared/types/navigation"
import { User } from "store/reducers/auth/default"
import { updateTask } from "store/actions/container/pure"
import { validateAccessToken } from "store/actions/auth/sideEffects"
import { history } from "store/history"
import { FractalIPC } from "shared/types/ipc"
import SplashScreenComponent from "pages/login/components/splashScreen/splashScreen"
import RedirectComponent from "pages/login/components/redirect/redirect"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"
import {
    ADD_LOGIN_TOKEN,
    SUBSCRIBE_USER_ACCESS_TOKEN,
    DELETE_USER_TOKENS,
} from "shared/constants/graphql"
import { updateAuthFlow } from "store/actions/auth/pure"


import styles from "pages/login/login.css"

export const Login = (props: {
    userID: string
    accessToken: string
    loginToken: string
    dispatch: Dispatch
}) => {
    /*
        Login page that redirects user to a browser and validates the returned from the browser
 
        Arguments:
            userID: User ID
            accessToken: Access token  
    */

    const { userID, accessToken, loginToken, dispatch } = props

    const [buttonClicked, setButtonClicked] = useState(false)
    const [localAccessToken, setLocalAccessToken] = useState("")

    const ipc = require("electron").ipcRenderer

    const [addLogin] = useMutation(ADD_LOGIN_TOKEN)
    const { data, loading, error } = useSubscription(
        SUBSCRIBE_USER_ACCESS_TOKEN,
        {
            variables: { loginToken: loginToken },

        }
    )
    const [deleteTokens] = useMutation(DELETE_USER_TOKENS, {
        context: {
            headers: {
                Login: loginToken,

            },
        },
    })

    // const ipc = require("electron").ipcRenderer
    // ipc.on(FractalIPC.CUSTOM_URL, (_: any, customURL: string) => {
    //     if (customURL && customURL.toString().includes("fractal://")) {
    //         customURL = `fractal://${customURL.split("fractal://")[1]}`
    //         // Convert URL to URL object so it can be parsed
    //         const urlObj = new URL(customURL.toString())
    //         urlObj.protocol = "https"

    //         // Check to see if this is an auth request or an external app authentication
    //         // const websiteAccessToken = urlObj.searchParams.get("accessToken")

    //         // if (websiteAccessToken) {
    //         //     dispatch(validateAccessToken(websiteAccessToken))
    //         // }
    //     }
    // })

    const crypto = require("crypto")
    async function generateToken() {
        /*
            Generate a unique one-time use login token
 
            Arguments:
                none     
        */
        const buffer = await new Promise((resolve, reject) => {
            crypto.randomBytes(128, (err, buffer) => {
                if (err) {
                    reject("error generating token")
                }
                resolve(buffer)
            })
        })
        return buffer.toString("hex")
    }

    const handleLogin = () => {
        setButtonClicked(true)
        generateToken().then((token) => {
            const loginToken: string = Date.now() + token
            dispatch(updateAuthFlow({loginToken: loginToken}))
            addLogin({
                variables: {
                    object: {
                        login_token: loginToken,
                        access_token: null,
                    },
                },
            })
                .then(() => {
                    openExternal(
                        `${config.url.FRONTEND_URL}/auth/loginToken=${loginToken}`
                    )

                    //ipc.sendSync(FractalIPC.LOAD_BROWSER)
                })
                .catch((error) => {
                    throw error
                })
        })
    }
    useEffect(() => {
        if (error) {
            console.log(error)
        } else if (loading) {
            console.log("loading!")
        } else {
            const accessToken =
                data && data.tokens && data.tokens[0]
                    ? data.tokens[0].access_token
                    : null
            if (accessToken) {
                dispatch(validateAccessToken(accessToken))
                deleteTokens({
                    variables: {
                        loginToken: loginToken,
                    },
                })
            }
        }
    }, [data, loading, error])

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
        ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, true)
    }, [])

    useEffect(() => {
        if (userID && accessToken) {
            dispatch(updateTask({ shouldLaunchProtocol: true }))
            history.push(FractalRoute.LAUNCHER)
        }
    }, [accessToken, userID])

    return (
        <div className={styles.loginWrapper}>
            <ChromeBackground />
            <div className={styles.loginCenter}>
                {!buttonClicked ? (
                    <div>
                        <SplashScreenComponent onClick={handleLogin} />
                    </div>
                ) : (
                    <div style={{ position: "relative", left: "10%" }}>
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

const mapStateToProps = (state: { AuthReducer: { user: User, authFlow: {loginToken: string} } }) => {
    return {
        userID: state.AuthReducer.user.userID,
        accessToken: state.AuthReducer.user.accessToken,
        loginToken: state.AuthReducer.authFlow.loginToken
    }
}

export default connect(mapStateToProps)(Login)
