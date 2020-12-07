import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"
import { useSpring, animated } from "react-spring"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import { FractalRoute } from "shared/types/navigation"
import { updateApps } from "store/actions/pure"
import { history } from "store/history"

const Installing = (props: {
    dispatch: Dispatch
    notInstalled: string[]
    installed: string[]
}) => {
    const { dispatch, notInstalled, installed } = props

    const [currentApp, setCurrentApp] = useState("")
    const percentDownloaded =
        installed.length / (notInstalled.length + installed.length)
    const progressBar = useSpring({ width: percentDownloaded * 600 })

    const handleDone = () => {
        history.push(FractalRoute.DASHBOARD)
    }

    const sleep = (ms: number) => {
        return new Promise((resolve) => setTimeout(resolve, ms))
    }

    useEffect(() => {
        if (installed.length === 0) {
            setCurrentApp(notInstalled[0])
            const newNotInstalled = Object.assign([], notInstalled)
            const newInstalled = Object.assign([], installed)
            newInstalled.push(notInstalled[0])
            newNotInstalled.shift()
            dispatch(
                updateApps({
                    notInstalled: newNotInstalled,
                    installed: newInstalled,
                })
            )
        } else if (notInstalled.length > 0) {
            sleep(5000)
                .then(() => {
                    setCurrentApp(notInstalled[0])
                    const newNotInstalled = Object.assign([], notInstalled)
                    const newInstalled = Object.assign([], installed)
                    newInstalled.push(notInstalled[0])
                    newNotInstalled.shift()
                    dispatch(
                        updateApps({
                            notInstalled: newNotInstalled,
                            installed: newInstalled,
                        })
                    )
                    return null
                })
                .catch((err) => {
                    throw err
                })
        }
    }, [notInstalled, installed])

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

export const mapStateToProps = <T extends {}>(state: T) => {
    return {
        notInstalled: state.MainReducer.apps.notInstalled,
        installed: state.MainReducer.apps.installed,
    }
}

export default connect(mapStateToProps)(Installing)
