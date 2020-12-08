import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"
import { useSpring, animated } from "react-spring"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import { FractalRoute } from "shared/types/navigation"
import { FractalApp } from "shared/types/ui"
import { OperatingSystem } from "shared/types/client"
import { createWindowsShortcuts } from "shared/utils/files/shortcuts"
import { history } from "store/history"

const Installing = (props: {
    onboardApps: FractalApp[]
    apps: FractalApp[]
    clientOS: OperatingSystem
}) => {
    const { apps, onboardApps, clientOS } = props

    const [currentApp, setCurrentApp] = useState("")
    const [progress, setProgress] = useState(0)
    const [doneInstalling, setDoneInstalling] = useState(false)

    const appsLength = apps && apps.length > 0 ? apps.length : 1
    const progressBar = useSpring({ width: (progress / appsLength) * 600 })

    const handleDone = (): void => {
        history.push(FractalRoute.DASHBOARD)
    }

    const createShortcutsWrapper = async (): Promise<void> => {
        const results = []

        for (let i = 0; i < onboardApps.length; i += 1) {
            const app = onboardApps[i]
            setCurrentApp(app.app_id)
            if (clientOS === OperatingSystem.WINDOWS) {
                results.push(createWindowsShortcuts(app))
            }
            setProgress(progress + 1)
        }

        await Promise.all(results)
        setDoneInstalling(true)
    }

    useEffect(() => {
        if (onboardApps && onboardApps.length > 0) {
            createShortcutsWrapper()
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
                    {doneInstalling ? (
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
            clientOS: OperatingSystem
        }
    }
}) => {
    return {
        onboardApps: state.MainReducer.client.onboardApps,
        apps: state.MainReducer.client.apps,
        clientOS: state.MainReducer.client.clientOS,
    }
}

export default connect(mapStateToProps)(Installing)
