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
    clientOS: OperatingSystem
}) => {
    const { onboardApps, clientOS } = props

    const [currentApp, setCurrentApp] = useState("")
    const [progress, setProgress] = useState(0)

    const appsLength =
        onboardApps && onboardApps.length > 0 ? onboardApps.length : 1
    const progressBar = useSpring({ width: (progress / appsLength) * 600 })

    const handleDone = (): void => {
        history.push(FractalRoute.DASHBOARD)
    }

    const createShortcutsWrapper = async (): Promise<any> => {
        if (clientOS === OperatingSystem.WINDOWS) {
            await onboardApps.reduce(
                async (previousPromise: Promise<any>, nextApp: FractalApp) => {
                    await previousPromise
                    setCurrentApp(nextApp.app_id)
                    setProgress(progress + 1)
                    return createWindowsShortcuts(nextApp)
                },
                Promise.resolve()
            )
            setProgress(appsLength)
        }
    }

    useEffect(() => {
        console.log("progres", progress)
        console.log("onboard app", onboardApps.length)
    }, [progress])

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
                        is complete.
                    </div>
                    <div className={styles.installingBar}>
                        <animated.div
                            className={styles.progress}
                            style={progressBar}
                        />
                    </div>
                    {progress === appsLength ? (
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
            clientOS: OperatingSystem
        }
    }
}) => {
    return {
        onboardApps: state.MainReducer.client.onboardApps,
        clientOS: state.MainReducer.client.clientOS,
    }
}

export default connect(mapStateToProps)(Installing)
