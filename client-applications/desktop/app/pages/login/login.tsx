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

import { updateClient, updateAuth } from "store/actions/pure"
import { findDPI } from "pages/login/constants/helpers"
import { setAWSRegion } from "shared/utils/exec"
import { openExternal } from "shared/utils/helpers"
import { config } from "shared/constants/config"

const Login = (props: any) => {
    const { dispatch, os, loginWarning } = props
    const [loggingIn, setLoggingIn] = useState(false)
    const [version, setVersion] = useState("1.0.0")

    const handleLoginUser = () => {
        setLoggingIn(true)
        setAWSRegion().then((region) => {
            dispatch(updateClient({ region: region }))
            dispatch(updateAuth({ loginWarning: false, loginMessage: null }))
            openExternal(
                config.url.FRONTEND_URL + "/auth/bypass?callback=fractal://"
            )
        })
    }

    useEffect(() => {
        const appVersion = require("../../package.json").version
        const dpi = findDPI()
        const os = require("os")
        dispatch(updateClient({ os: os.platform(), dpi: dpi }))
        setVersion(appVersion)
    }, [])

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
            <div className={styles.removeDrag}>
                <div className={styles.landingHeader}>
                    <div className={styles.landingHeaderLeft}>
                        <span className={styles.logoTitle}>Fractal</span>
                    </div>
                </div>
                <div style={{ marginTop: loginWarning ? 0 : 50 }}>
                    <div className={styles.loginContainer}>
                        <BackgroundView />
                        <div className={styles.welcomeBack}>Welcome Back!</div>
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
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        loginWarning: state.MainReducer.auth.loginWarning,
        loginMessage: state.MainReducer.auth.loginMessage,
        os: state.MainReducer.client.os,
    }
}

export default connect(mapStateToProps)(Login)
