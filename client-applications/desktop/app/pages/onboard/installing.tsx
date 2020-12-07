import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"
import { useSpring, animated } from "react-spring"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import { FractalRoute } from "shared/types/navigation"
import { FractalApp } from "shared/types/ui"
import { OperatingSystem } from "shared/types.client"
import { updateClient } from "store/actions/pure"
import { createWindowsShortcut } from "shared/utils/shortcuts"
import { history } from "store/history"

const MAX_PROGRESS = 1

const Installing = (props: {
    onboardApps: FractalApp[]
    apps: FractalApp[]
    dispatch: Dispatch<any>
}) => {
    const { apps, onboardApps, dispatch } = props

    const [currentApp, setCurrentApp] = useState("")
    const [progress, setProgress] = useState(0)

    const progressBar = useSpring({ width: progress * 600 })

    const handleDone = () => {
        history.push(FractalRoute.DASHBOARD)
    }

    const createShortcutWrapper = async (app: FractalApp) => {
        await createWindowsShortcut(app)
    }

    useEffect(() => {
        for (let i = 0; i < onboardApps.length; i += 1) {
            const app = onboardApps[i]
            createShortcutWrapper(app)
        }
    }, [onboardApps])

    return (
        <div className={styles.container} data-tid="container">
            <Version />
            <TitleBar />
            <div className={styles.removeDrag}>
                <div className={styles.installingContainer}>
                    <h2>Your apps are installing.</h2>
                    <div className={styles.subtext} style={{ marginTop: 50 }}>
                        Please do not close this window until your installation
                        is complete. This will take no more than a few minutes.
                    </div>
                    <div className={styles.installingBar}>
                        <animated.div
                            className={styles.progress}
                            style={progressBar}
                        />
                    </div>
                    {percentDownloaded === 1 ? (
                        <>
                            <div className={styles.installingText}>
                                Done installing.
                            </div>
                            <button
                                type="button"
                                className={styles.enterButton}
                                onClick={handleDone}
                                style={{ marginTop: 100 }}
                            >
                                GO TO APP STORE
                            </button>
                        </>
                    ) : (
                        <div className={styles.installingText}>
                            Installing {currentApp}...
                        </div>
                    )}
                </div>
            </div>
        </div>
    )
}

export const mapStateToProps = (state: {
    MainReducer: {
        client: {
            onboardApps: FractalApp[]
            apps: FractalApp[]
        }
    }
}) => {
    return {
        onboardApps: state.MainReducer.client.apps,
        apps: state.MainReducer.client.apps,
    }
}

export default connect(mapStateToProps)(Installing)
