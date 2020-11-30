import React, { useState } from "react"
import { connect } from "react-redux"
import styles from "styles/login.css"
import PuffLoader from "react-spinners/PuffLoader"

import { updateClient, updateAuth } from "store/actions/pure"
import { setAWSRegion } from "shared/utils/exec"
import { openExternal } from "shared/utils/helpers"
import { config } from "shared/constants/config"

const LoginView = (props: any) => {
    const { dispatch } = props
    const [loggingIn, setLoggingIn] = useState(false)

    const handleLoginUser = () => {
        setLoggingIn(true)
        setAWSRegion().then((region) => {
            dispatch(updateClient({ region: region }))
            dispatch(updateAuth({ loginWarning: false, loginMessage: null }))
            openExternal(
                config.url.FRONTEND_URL + "/auth/bypass?callback=fractal://auth"
            )
        })
    }

    if (!loggingIn) {
        return (
            <div style={{ marginTop: 150 }}>
                <div className={styles.welcomeBack}>
                    Welcome Back! <br />
                    You've been signed out.
                </div>
                <div style={{ marginTop: 30 }}>
                    <button
                        onClick={handleLoginUser}
                        type="button"
                        className={styles.loginButton}
                        id="login-button"
                    >
                        LOG IN
                    </button>
                </div>
            </div>
        )
    } else {
        return (
            <div>
                <PuffLoader css={"marginTop: 300px; margin: auto;"} size={75} />
                <div style={{ margin: "auto", marginTop: 50, maxWidth: 450 }}>
                    A browser window should open momentarily where you can
                    login. Click{" "}
                    <span
                        onClick={handleLoginUser}
                        style={{ fontWeight: "bold", cursor: "pointer" }}
                    >
                        here
                    </span>{" "}
                    if it doesn't appear. Once you've logged in, this page will
                    automatically redirect.
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: any) {
    return {}
}

export default connect(mapStateToProps)(LoginView)
