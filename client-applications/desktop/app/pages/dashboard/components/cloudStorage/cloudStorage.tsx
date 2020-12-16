/* eslint-disable import/no-named-as-default */

import React, { Dispatch, useState } from "react"
import { Row } from "react-bootstrap"
import { connect } from "react-redux"

import CloudStorageAlert from "pages/dashboard/components/cloudStorage/components/cloudStorageAlert"
import styles from "pages/dashboard/views/settings/settings.css"

import { config } from "shared/constants/config"
import { openExternal } from "shared/utils/general/helpers"
import { disconnectApp } from "store/actions/sideEffects"
import { ExternalApp } from "store/reducers/types"

const CloudStorage = (props: {
    dispatch: Dispatch<any>
    accessToken: string
    externalApps: ExternalApp[]
    connectedApps: string[]
}) => {
    const { dispatch, accessToken, externalApps, connectedApps } = props

    // name of app most recently connect to from settings, used to render alerts correctly
    const [connectedFromSettings, setConnectedFromSettings] = useState("")

    return (
        <>
            <h2 className={styles.title} style={{ marginTop: 40 }}>
                Cloud Storage
            </h2>
            <div className={styles.subtitle}>
                Access, manage, edit, and share files from any of the following
                cloud storage services within your streamed apps by connecting
                your cloud drives to your Fractal account.
            </div>
            {externalApps.map((externalApp: ExternalApp) => (
                <Row className={styles.row} key={externalApp.code_name}>
                    <div style={{ width: "100%", display: "flex" }}>
                        <div style={{ width: "75%" }}>
                            <div className={styles.header}>
                                <img
                                    alt="External app"
                                    src={externalApp.image_s3_uri}
                                    className={styles.cloudStorageIcon}
                                />
                                {externalApp.display_name}
                            </div>
                            <div className={styles.text}>
                                By using this service through Fractal, you are
                                agreeing to their{" "}
                                <button
                                    type="button"
                                    className={styles.tosLink}
                                    onClick={() =>
                                        openExternal(externalApp.tos_uri)
                                    }
                                >
                                    terms of service.
                                </button>
                            </div>
                        </div>
                        <div style={{ width: "25%", alignSelf: "center" }}>
                            <div
                                style={{
                                    float: "right",
                                }}
                            >
                                {connectedApps.indexOf(externalApp.code_name) >
                                -1 ? (
                                    <button
                                        type="button"
                                        className={styles.connectButton}
                                        onClick={() =>
                                            dispatch(
                                                disconnectApp(
                                                    externalApp.code_name
                                                )
                                            )
                                        }
                                    >
                                        Disconnect
                                    </button>
                                ) : (
                                    <button
                                        type="button"
                                        className={styles.connectButton}
                                        onClick={() => {
                                            setConnectedFromSettings(
                                                externalApp.code_name
                                            )
                                            openExternal(
                                                `${config.url.WEBSERVER_URL}/oauth/authorize?external_app=${externalApp.code_name}&access_token=${accessToken}`
                                            )
                                        }}
                                    >
                                        Connect
                                    </button>
                                )}
                            </div>
                        </div>
                    </div>
                    <CloudStorageAlert
                        externalApp={externalApp}
                        connectedFromSettings={
                            connectedFromSettings === externalApp.code_name
                        }
                    />
                </Row>
            ))}
        </>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        auth: { accessToken: string }
        apps: {
            externalApps: ExternalApp[]
            connectedApps: string[]
        }
    }
}) => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
        externalApps: state.MainReducer.apps.externalApps,
        connectedApps: state.MainReducer.apps.connectedApps,
    }
}

export default connect(mapStateToProps)(CloudStorage)
