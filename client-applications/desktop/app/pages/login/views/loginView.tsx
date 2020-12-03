import React, { useState, ChangeEvent, KeyboardEvent } from "react"
import { connect } from "react-redux"
import styles from "pages/login/login.css"
import PuffLoader from "react-spinners/PuffLoader"

import { updateClient, updateAuth } from "store/actions/pure"
import { setAWSRegion } from "shared/utils/exec"
import { openExternal } from "shared/utils/helpers"
import { config } from "shared/constants/config"
import { FractalKey } from "shared/types/input"

const LoginView = <T extends {}>(props: T) => {
    const { dispatch } = props
    const [loggingIn, setLoggingIn] = useState(false)
    const [accessToken, setAccessToken] = useState("")

    const handleLoginUser = () => {
        setLoggingIn(true)
        setAWSRegion()
            .then((region) => {
                dispatch(updateClient({ region: region }))
                dispatch(
                    updateAuth({ loginWarning: false, loginMessage: null })
                )
                openExternal(
                    `${config.url.FRONTEND_URL}/auth/bypass?callback=fractal://auth`
                )
                return null
            })
            .catch((err) => {
                throw err
            })
    }

    const changeAccessToken = (evt: ChangeEvent) => {
        evt.persist()
        setAccessToken(evt.target.value)
    }

    const onKeyPress = (evt: KeyboardEvent) => {
        if (evt.key === FractalKey.ENTER) {
            dispatch(updateAuth({ candidateAccessToken: accessToken }))
        }
    }

    if (!loggingIn) {
        return (
            <div style={{ marginTop: 150 }}>
                <div className={styles.welcomeBack}>Welcome Back!</div>
                <div>You&lsquo;ve been signed out.</div>
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
    }
    return (
        <div style={{ marginTop: 150 }}>
            <PuffLoader css="marginTop: 300px; margin: auto;" size={75} />
            <div style={{ margin: "auto", marginTop: 50, maxWidth: 450 }}>
                A browser window should open momentarily where you can login.
                Click{" "}
                <span
                    role="button"
                    tabIndex={0}
                    onClick={handleLoginUser}
                    onKeyDown={handleLoginUser}
                    style={{ fontWeight: "bold", cursor: "pointer" }}
                >
                    here
                </span>{" "}
                if it doesn&lsquo;t appear. Once you&lsquo;ve logged in, this
                page will automatically redirect.
            </div>
            {config.sentryEnv === "development" && (
                <div style={{ marginTop: 40 }}>
                    <input
                        type="text"
                        onChange={changeAccessToken}
                        onKeyPress={onKeyPress}
                        style={{
                            width: 400,
                            border: "none",
                            boxShadow: "none",
                            margin: "auto",
                            background: "rgba(237, 240, 247, 0.75)",
                            padding: "5px 20px",
                            borderRadius: 5,
                            outline: "none",
                        }}
                    />
                </div>
            )}
        </div>
    )
}

export const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(LoginView)
