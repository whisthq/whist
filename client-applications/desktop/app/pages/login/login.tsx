import React from "react"
import { connect } from "react-redux"

import TitleBar from "shared/components/titleBar"
import Version from "shared/components/version"
import BackgroundView from "shared/views/backgroundView"
import LoginView from "pages/login/views/loginView/loginView"

import styles from "pages/login/login.css"

const Login = (props: { loginWarning: string }) => {
    const { loginWarning } = props

    return (
        <div className={styles.container} data-tid="container">
            <TitleBar />
            <Version />
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

export const mapStateToProps = (state: {
    MainReducer: {
        auth: {
            loginWarning: boolean
        }
    }
}) => {
    return {
        loginWarning: state.MainReducer.auth.loginWarning,
    }
}

export default connect(mapStateToProps)(Login)
