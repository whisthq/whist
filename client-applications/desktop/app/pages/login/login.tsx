import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "styles/login.css"
import Titlebar from "react-electron-titlebar"
import { parse } from "url"

import BackgroundView from "pages/login/views/backgroundView"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import {
    faCircleNotch,
    faUser,
    faLock,
} from "@fortawesome/free-solid-svg-icons"
// import { FaGoogle } from "react-icons/fa"

import { history } from "store/configureStore"
import { updateClient, updateAuth } from "store/actions/pure"
import {
    googleLogin,
    loginUser,
    rememberMeLogin,
} from "store/actions/sideEffects"
import { config } from "shared/constants/config"
import { findDPI } from "pages/login/constants/helpers"
import { setAWSRegion } from "shared/utils/exec"

const Login = (props: any) => {
    const {
        dispatch,
        os,
        loginWarning,
        loginMessage,
        accessToken,
        launchURL,
    } = props
    const [username, setUsername] = useState("")
    const [password, setPassword] = useState("")
    const [loggingIn, setLoggingIn] = useState(false)
    const [version, setVersion] = useState("1.0.0")
    const [rememberMe, setRememberMe] = useState(false)
    const live = useState(true)

    const storage = require("electron-json-storage")

    const updateUsername = (evt: any) => {
        setUsername(evt.target.value)
    }

    const updatePassword = (evt: any) => {
        setPassword(evt.target.value)
    }

    const handleLoginUser = () => {
        setLoggingIn(true)
        setAWSRegion().then((region) => {
            dispatch(updateClient({ region: region }))
            dispatch(updateAuth({ loginWarning: false, loginMessage: null }))
            if (!rememberMe) {
                storage.set("credentials", {
                    username: "",
                    accessToken: "",
                    refreshToken: "",
                })
            }
            setUsername(username)
            setPassword(password)
            dispatch(loginUser(username.trim(), password, rememberMe))
        })
    }

    const loginKeyPress = (event: any) => {
        if (event.key === "Enter") {
            handleLoginUser()
        }
    }

    const handleGoogleLogin = () => {
        const { BrowserWindow } = require("electron").remote

        const authWindow = new BrowserWindow({
            width: 800,
            height: 600,
            show: false,
            "node-integration": false,
            "web-security": false,
        })
        const GOOGLE_CLIENT_ID = config.keys.GOOGLE_CLIENT_ID
        const GOOGLE_REDIRECT_URI = config.url.GOOGLE_REDIRECT_URI
        const authUrl = `https://accounts.google.com/o/oauth2/v2/auth?scope=openid%20profile%20email&openid.realm&include_granted_scopes=true&response_type=code&redirect_uri=${GOOGLE_REDIRECT_URI}&client_id=${GOOGLE_CLIENT_ID}`
        authWindow.loadURL(authUrl, { userAgent: "Chrome" })
        authWindow.show()

        const handleNavigation = (url: any) => {
            const query = parse(url, true).query
            if (query) {
                if (query.error) {
                    // dispatch(loginFailed(true))
                } else if (query.code) {
                    setLoggingIn(true)
                    authWindow.removeAllListeners("closed")
                    setImmediate(() => authWindow.close())
                    setAWSRegion().then(() => {
                        dispatch(googleLogin(query.code, rememberMe))
                    })
                }
            }
        }

        authWindow.webContents.on("will-navigate", (_: any, url: any) => {
            handleNavigation(url)
        })

        authWindow.webContents.on(
            "did-get-redirect-request",
            (_: any, oldUrl: any, newUrl: any) => {
                handleNavigation(newUrl)
            }
        )
    }

    const forgotPassword = () => {
        const { shell } = require("electron")
        shell.openExternal("https://www.tryfractal.com/reset")
    }

    const signUp = () => {
        const { shell } = require("electron")
        shell.openExternal("https://www.tryfractal.com/auth")
    }

    const changeRememberMe = (event: any) => {
        setRememberMe(event.target.checked)
    }

    useEffect(() => {
        dispatch(updateAuth({ loginWarning: false }))

        const appVersion = require("../../package.json").version
        const dpi = findDPI()
        const os = require("os")
        dispatch(updateClient({ os: os.platform(), dpi: dpi }))
        setVersion(appVersion)

        storage.get("credentials", (error: any, data: any) => {
            if (error) throw error

            if (data && Object.keys(data).length > 0) {
                if (
                    data.username &&
                    data.accessToken &&
                    data.refreshToken &&
                    live
                ) {
                    setLoggingIn(true)
                    dispatch(
                        updateAuth({
                            username: data.username,
                            accessToken: data.accessToken,
                            refreshToken: data.refreshToken,
                        })
                    )
                    setRememberMe(true)
                    setUsername(data.username)
                    dispatch(rememberMeLogin(data.username))
                }
            }
        })
    }, [])

    useEffect(() => {
        if (
            !launchURL &&
            props.username &&
            props.accessToken &&
            props.accessToken !== ""
        ) {
            history.push("/dashboard")
        }
    }, [props.username, accessToken])

    return (
        <div className={styles.container} data-tid="container">
            <div
                style={{
                    position: "absolute",
                    bottom: 15,
                    right: 15,
                    fontSize: 11,
                    color: "#D1D1D1",
                }}
            >
                Version: {version}
            </div>
            {os === "win32" ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className={styles.macTitleBar} />
            )}
            {live ? (
                <div className={styles.removeDrag}>
                    <div className={styles.landingHeader}>
                        <div className={styles.landingHeaderLeft}>
                            <span className={styles.logoTitle}>Fractal</span>
                        </div>
                        <div className={styles.landingHeaderRight}>
                            <button
                                type="button"
                                className={styles.signupButton}
                                id="signup-button"
                                onClick={signUp}
                            >
                                Sign Up
                            </button>
                        </div>
                    </div>
                    <div style={{ marginTop: loginWarning ? 0 : 50 }}>
                        <div className={styles.loginContainer}>
                            <BackgroundView />
                            <div className={styles.welcomeBack}>
                                Welcome Back!
                            </div>
                            {loginWarning && (
                                <div
                                    style={{
                                        textAlign: "center",
                                        fontSize: 12,
                                        color: "#f9000b",
                                        background: "rgba(253, 240, 241, 0.9)",
                                        width: "100%",
                                        padding: 15,
                                        borderRadius: 2,
                                        margin: "auto",
                                        marginBottom: 30,
                                        // width: 265,
                                    }}
                                >
                                    {loginMessage ? (
                                        <div>{loginMessage}</div>
                                    ) : (
                                        <div>
                                            Invalid credentials. If you lost
                                            your password, you can reset it on
                                            the&nbsp;
                                            <div
                                                onClick={forgotPassword}
                                                className={
                                                    styles.pointerOnHover
                                                }
                                                style={{
                                                    display: "inline",
                                                    fontWeight: "bold",
                                                    textDecoration: "underline",
                                                }}
                                            >
                                                website
                                            </div>
                                            .
                                        </div>
                                    )}
                                </div>
                            )}
                            <div className={styles.labelContainer}>
                                USERNAME
                            </div>
                            <div>
                                <FontAwesomeIcon
                                    icon={faUser}
                                    style={{
                                        color: "#555555",
                                        fontSize: 12,
                                    }}
                                    className={styles.inputIcon}
                                />
                                <input
                                    onKeyPress={loginKeyPress}
                                    onChange={updateUsername}
                                    type="text"
                                    className={styles.inputBox}
                                    placeholder={username ? username : ""}
                                    id="username"
                                />
                            </div>
                            <div className={styles.labelContainer}>
                                PASSWORD
                                <span
                                    className={styles.forgotButton}
                                    onClick={forgotPassword}
                                >
                                    FORGOT PASSWORD?
                                </span>
                            </div>
                            <div>
                                <FontAwesomeIcon
                                    icon={faLock}
                                    style={{
                                        color: "#555555",
                                        fontSize: 12,
                                    }}
                                    className={styles.inputIcon}
                                />
                                <input
                                    onKeyPress={loginKeyPress}
                                    onChange={updatePassword}
                                    type="password"
                                    className={styles.inputBox}
                                    placeholder={
                                        rememberMe && loggingIn
                                            ? "•••••••••"
                                            : ""
                                    }
                                    id="password"
                                />
                            </div>
                            <div style={{ marginBottom: 20 }}>
                                {loggingIn && !loginWarning ? (
                                    <button
                                        type="button"
                                        className={styles.processingButton}
                                        id="login-button"
                                        style={{
                                            opacity: 0.6,
                                            textAlign: "center",
                                        }}
                                    >
                                        <FontAwesomeIcon
                                            icon={faCircleNotch}
                                            spin
                                            style={{
                                                color: "#555555",
                                                width: 12,
                                                marginRight: 5,
                                                position: "relative",
                                                top: 0.5,
                                            }}
                                        />{" "}
                                        Processing
                                    </button>
                                ) : (
                                    <button
                                        onClick={handleLoginUser}
                                        type="button"
                                        className={styles.loginButton}
                                        id="login-button"
                                    >
                                        LOG IN
                                    </button>
                                )}
                                {/* <div style={{ marginBottom: 20 }}>
                                    {loggingIn && !loginWarning ? (
                                        <button
                                            type="button"
                                            className={styles.googleButton}
                                            id="google-button"
                                            style={{
                                                opacity: 0.6,
                                                textAlign: "center",
                                            }}
                                        >
                                            <FontAwesomeIcon
                                                icon={faCircleNotch}
                                                spin
                                                style={{
                                                    color: "white",
                                                    width: 12,
                                                    marginRight: 5,
                                                    position: "relative",
                                                    top: 0.5,
                                                }}
                                            />{" "}
                                            Processing
                                        </button>
                                    ) : (
                                        <button
                                            onClick={handleGoogleLogin}
                                            type="button"
                                            className={styles.googleButton}
                                            id="google-button"
                                        >
                                            <FaGoogle
                                                style={{
                                                    fontSize: 16,
                                                    marginRight: 10,
                                                    position: "relative",
                                                    bottom: 3,
                                                }}
                                            />
                                            Login with Google
                                        </button>
                                    )}
                                </div> */}
                            </div>
                            <div
                                style={{
                                    marginTop: 25,
                                    display: "flex",
                                    justifyContent: "center",
                                    alignItems: "center",
                                }}
                            >
                                <label className={styles.termsContainer}>
                                    <input
                                        type="checkbox"
                                        onChange={changeRememberMe}
                                        onKeyPress={loginKeyPress}
                                        checked={rememberMe}
                                    />
                                    <span className={styles.checkmark} />
                                </label>

                                <div style={{ fontSize: 12, color: "#555555" }}>
                                    Remember Me
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            ) : (
                <div
                    style={{
                        lineHeight: 1.5,
                        margin: "150px auto",
                        maxWidth: 400,
                    }}
                >
                    {" "}
                    We are currently pushing out a critical Linux update. Your
                    app will be back online very soon. We apologize for the
                    inconvenience!
                </div>
            )}
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        username: state.MainReducer.auth.username,
        loginWarning: state.MainReducer.auth.loginWarning,
        loginMessage: state.MainReducer.auth.loginMessage,
        accessToken: state.MainReducer.auth.accessToken,
        refreshToken: state.MainReducer.auth.refreshToken,
        os: state.MainReducer.client.os,
    }
}

export default connect(mapStateToProps)(Login)
