import React from "react"
import { connect } from "react-redux"
import styles from "pages/onboard/onboard.css"

import Version from "shared/components/version"
import TitleBar from "shared/components/titleBar"
import StorageService from "pages/onboard/components/storageService"
import { FractalRoute } from "shared/types/navigation"
import { history } from "store/history"
import { ExternalAppType } from "store/reducers/types"

const Storage = (props: {
    externalApps: ExternalAppType[]
    connectedApps: string[]
}) => {
    const { externalApps, connectedApps } = props

    const handleDone = () => {
        history.push(FractalRoute.DASHBOARD)
    }

    return (
        <div className={styles.container} data-tid="container">
            <Version />
            <TitleBar />
            <div className={styles.removeDrag}>
                <div className={styles.appsContainer}>
                    <h2 className={styles.appsTitle}>
                        Connect your cloud drive.
                    </h2>
                    <div className={styles.subtext}>
                        Access, manage, edit, and share files from any of the
                        following cloud storage services within your streamed
                        apps by connecting your cloud drives to your Fractal
                        account. Fractal will not store any of your data, and
                        you can disconnect your accounts at any time.
                    </div>
                    <div className={styles.servicesWrapper}>
                        {externalApps.map((externalApp: any) => (
                            <StorageService
                                externalApp={externalApp}
                                connected={
                                    connectedApps.indexOf(
                                        externalApp.code_name
                                    ) > -1
                                }
                                key={externalApp.code_name}
                            />
                        ))}
                    </div>
                    <button
                        type="button"
                        className={styles.enterButton}
                        onClick={handleDone}
                    >
                        GO TO APP STORE
                    </button>
                </div>
            </div>
        </div>
    )
}

export const mapStateToProps = <T extends {}>(state: T) => {
    return {
        externalApps: state.MainReducer.apps.external,
        connectedApps: state.MainReducer.apps.connected,
    }
}

export default connect(mapStateToProps)(Storage)
