import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "styles/login.css"
import Titlebar from "react-electron-titlebar"
import { parse } from "url"
import { useQuery } from "@apollo/client"

import UpdateScreen from "pages/dashboard/components/update"
import BackgroundView from "pages/login/views/backgroundView"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import {
    faCircleNotch,
    faUser,
    faLock,
} from "@fortawesome/free-solid-svg-icons"

import { FaGoogle } from "react-icons/fa"

import { updateClient, updateAuth } from "store/actions/pure"
import {
    googleLogin,
    loginUser,
    rememberMeLogin,
} from "store/actions/sideEffects"
import { debugLog } from "shared/utils/logging"
import { config } from "shared/constants/config"
import { fetchContainer } from "store/actions/sideEffects"
import { execChmodUnix } from "shared/utils/exec"
import { GET_FEATURED_APPS } from "shared/constants/graphql"
import { checkActive, urlToApp, findDPI } from "pages/login/constants/helpers"

const Login = (props: any) => {
    const {
        dispatch,
        os,
        loginWarning,
        loginMessage,
        launchImmediately,
        launchURL,
        accessToken,
        refreshToken,
    } = props

    const [username, setUsername] = useState("")
    const [password, setPassword] = useState("")
    const [loggingIn, setLoggingIn] = useState(false)
    const [version, setVersion] = useState("1.0.0")
    const [rememberMe, setRememberMe] = useState(false)
    const [updatePingReceived, setUpdatePingReceived] = useState(false)
    const [needsAutoupdate, setNeedsAutoupdate] = useState(false)
    const [fetchedCredentials, setFetchedCredentials] = useState(false)
    const [launches, setLaunches] = useState(0)
    const live = useState(true)

    const storage = require("electron-json-storage")

    const { data } = useQuery(GET_FEATURED_APPS)
    const featuredAppData = data
        ? data.hardware_supported_app_images.filter(checkActive)
        : []

    const updateUsername = (evt: any) => {
        setUsername(evt.target.value)
    }

    const updatePassword = (evt: any) => {
        setPassword(evt.target.value)
    }

    const setAWSRegion = () => {
        return new Promise((resolve, reject) => {
            const { spawn } = require("child_process")
            const os = require("os")
            var appRootDir = require("electron").remote.app.getAppPath()
            var executable = ""
            var path = ""

            if (os.platform() === "darwin") {
                path = appRootDir + "/binaries/"
                path = path.replace("/Resources/app.asar", "")
                path = path.replace("/app", "")
                executable = "./awsping_osx"
            } else if (os.platform() === "linux") {
                path = process.cwd() + "/binaries/"
                path = path.replace("/release", "")
                executable = "./awsping_linux"
            } else if (os.platform() === "win32") {
                path = appRootDir + "\\binaries"
                path = path.replace("\\resources\\app.asar", "")
                path = path.replace("\\app", "")
                executable = "awsping_windows.exe"
            } else {
                console.log(
                    `no suitable os found, instead got ${os.platform()}`
                )
            }

            execChmodUnix("chmod +x awsping_osx", path, os.platform()).then(
                () => {
                    const regions = spawn(executable, ["-n", "3"], {
                        cwd: path,
                    }) // ping via TCP
                    regions.stdout.setEncoding("utf8")

                    regions.stdout.on("data", (data: any) => {
                        // console.log(data)
                        // Gets the line with the closest AWS region, and replace all instances of multiple spaces with one space
                        const line = data.split(/\r?\n/)[0].replace(/  +/g, " ")
                        const items = line.split(" ")
                        // In case data is split and sent separately, only use closest AWS region which has index of 0
                        if (items[1] == "1.") {
                            const region = items[2].slice(1, -1)
                            // console.log("Ping detected " + region.toString())
                            dispatch(updateClient({ region: region }))
                        } else {
                            debugLog("Late packet")
                        }

                        resolve()
                    })
                }
            )
        })
    }

    const handleLoginUser = () => {
        setLoggingIn(true)
        setAWSRegion().then(() => {
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
                    authWindow.removeAllListeners("closed")
                    setImmediate(() => authWindow.close())
                    setLoggingIn(true)
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

        const ipc = require("electron").ipcRenderer

        ipc.on("update", (_: any, update: any) => {
            setUpdatePingReceived(true)
            setNeedsAutoupdate(update)
        })

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
                    dispatch(
                        updateAuth({
                            username: data.username,
                            accessToken: data.accessToken,
                            refreshToken: data.refreshToken,
                        })
                    )
                    setRememberMe(true)
                    setUsername(data.username)
                    setLoggingIn(true)
                    setFetchedCredentials(true)
                    dispatch(rememberMeLogin(data.username))
                }
            }
        })
    }, [])

    useEffect(() => {
        if (
            updatePingReceived &&
            !needsAutoupdate &&
            (username || props.username) &&
            accessToken &&
            refreshToken &&
            launchImmediately &&
            featuredAppData.length > 0
        ) {
            setLaunches(launches + 1)
        }
    }, [
        updatePingReceived,
        fetchedCredentials,
        props.username,
        accessToken,
        featuredAppData,
    ])

    useEffect(() => {
        if (launches === 1) {
            setAWSRegion().then(() => {
                const { app_id, url } = Object(
                    urlToApp(launchURL.toLowerCase(), featuredAppData)
                )
                dispatch(fetchContainer(app_id, url))
            })
        }
    }, [launches])

    return (
        <div className={styles.container} data-tid="container">
            <UpdateScreen />
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
