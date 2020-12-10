import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"
import { useSpring, animated } from "react-spring"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import { FractalRoute } from "shared/types/navigation"
import { FractalApp } from "shared/types/ui"
import { OperatingSystem } from "shared/types/client"
import { createShortcut } from "shared/utils/files/shortcuts"
import { history } from "store/history"

const Installing = (props: {
    onboardApps: FractalApp[]
    clientOS: OperatingSystem
}) => {
    const { onboardApps, clientOS } = props

    const [currentApp, setCurrentApp] = useState("")
    const [progress, setProgress] = useState(1)

    const appsLength =
        onboardApps && onboardApps.length > 0 ? onboardApps.length : 1
    const progressBar = useSpring({
        config: { friction: 0, mass: 0.5 },
        to: { width: (progress / appsLength) * 600 },
        from: { width: ((progress - 1) / appsLength) * 600 },
    })

    const createShortcutWrapper = async (): Promise<any> => {
        let currentProgress = 1
        await onboardApps.reduce(
            async (previousPromise: Promise<any>, nextApp: FractalApp) => {
                await previousPromise
                setCurrentApp(nextApp.app_id)
                setProgress(currentProgress)
                currentProgress = currentProgress + 1
                return createShortcut(nextApp)
            },
            Promise.resolve()
        )
        setProgress(appsLength)
    }

    useEffect(() => {
        if (onboardApps && onboardApps.length > 0) {
            createShortcutWrapper()
        } else {
            history.push(FractalRoute.DASHBOARD)
        }
    }, [onboardApps])

    return (
        <div className={styles.container} data-tid="container">
            <Version />
            <TitleBar />
            <div className={styles.removeDrag}>
                <div className={styles.installingContainer}>
                    {progress === appsLength && onboardApps.length > 0 ? (
                        <>
                            <h2 style={{ marginTop: 225 }}>
                                Success! One last step.
                            </h2>
                            <div
                                className={styles.installingText}
                                style={{ fontWeight: "normal" }}
                            >
                                In your{" "}
                                {clientOS === OperatingSystem.WINDOWS
                                    ? "Windows"
                                    : "Mac"}{" "}
                                search bar, type{" "}
                                <span className={styles.command}>
                                    Fractalized
                                </span>{" "}
                                to pull up your apps. For example, if you open{" "}
                                <span className={styles.command}>
                                    Fractalized {onboardApps[0].app_id}
                                </span>
                                , this page will refresh!
                            </div>
                        </>
                    ) : (
                        <>
                            <h2>Your apps are installing.</h2>
                            <div
                                className={styles.subtext}
                                style={{ marginTop: 50 }}
                            >
                                Please do not close this window until your
                                installation is complete.
                            </div>
                            <div className={styles.installingBar}>
                                <animated.div
                                    className={styles.progress}
                                    style={progressBar}
                                />
                            </div>
                            <div className={styles.installingText}>
                                Installing {currentApp}...
                            </div>
                        </>
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
