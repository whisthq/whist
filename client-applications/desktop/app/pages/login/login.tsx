import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "styles/login.css"
import Titlebar from "react-electron-titlebar"

import BackgroundView from "pages/login/views/backgroundView"
import LoginView from "pages/login/views/loginView"

const Login = (props: any) => {
    const { dispatch, os, loginWarning } = props
    const [version, setVersion] = useState("1.0.0")

    useEffect(() => {
        const appVersion = require("../../package.json").version
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
                        <LoginView />
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
