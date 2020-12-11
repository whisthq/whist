import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import BackgroundView from "shared/views/backgroundView"
import { FractalRoute } from "shared/types/navigation"
import { history } from "store/history"

const Welcome = (props: { username: string; name: string }) => {
    const { username, name } = props

    const [welcomeMessage, setWelcomeMessage] = useState("Welcome")

    useEffect(() => {
        if (name) {
            setWelcomeMessage(`Welcome, ${name}`)
        } else if (username) {
            setWelcomeMessage(`Welcome, ${username}`)
        }
    }, [])

    const handleEnter = () => {
        history.push(FractalRoute.ONBOARD_APPS)
    }

    return (
        <div className={styles.container} data-tid="container">
            <Version />
            <TitleBar />
            <div className={styles.removeDrag}>
                <div className={styles.landingHeader}>
                    <div className={styles.landingHeaderLeft}>
                        <span className={styles.logoTitle}>Fractal</span>
                    </div>
                </div>
                <BackgroundView />
                <div className={styles.welcomeContainer}>
                    <h2 style={{ fontWeight: "normal" }}>
                        {welcomeMessage}. You have VIP Access.
                    </h2>
                    <button
                        type="button"
                        className={styles.enterButton}
                        onClick={handleEnter}
                    >
                        TAKE ME INSIDE
                    </button>
                </div>
            </div>
        </div>
    )
}

export const mapStateToProps = <T extends {}>(state: T) => {
    return {
        username: state.MainReducer.auth.username,
        name: state.MainReducer.auth.name,
    }
}

export default connect(mapStateToProps)(Welcome)
