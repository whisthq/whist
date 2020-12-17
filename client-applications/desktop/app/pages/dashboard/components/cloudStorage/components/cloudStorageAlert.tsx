/* eslint-disable import/no-named-as-default */

import React, { useState, useEffect, Dispatch } from "react"
import { Alert } from "react-bootstrap"
import { connect } from "react-redux"

import { AlertType } from "pages/dashboard/constants/alertTypes"
import styles from "pages/dashboard/views/settings/settings.css"

import { updateApps } from "store/actions/pure"
import { ExternalApp } from "store/reducers/types"

const CloudStorageAlert = (props: {
    dispatch: Dispatch<any>
    externalApp: ExternalApp
    connectedFromSettings: boolean
    authenticated: string
    disconnected: string
    disconnectWarning: string
}) => {
    const {
        dispatch,
        externalApp,
        connectedFromSettings,
        authenticated,
        disconnected,
        disconnectWarning,
    } = props

    const [alertType, setAlertType] = useState("")

    useEffect(() => {
        if (authenticated === externalApp.code_name) {
            setAlertType(AlertType.AUTHENTICATED)
            dispatch(updateApps({ authenticated: null }))
        }
    }, [authenticated, externalApp])

    useEffect(() => {
        if (disconnected === externalApp.code_name) {
            setAlertType(AlertType.DISCONNECTED)
            dispatch(updateApps({ disconnected: null }))
        }
    }, [disconnected, externalApp])

    useEffect(() => {
        if (disconnectWarning === externalApp.code_name) {
            setAlertType(AlertType.DISCONNECT_WARNING)
            dispatch(updateApps({ disconnectWarning: null }))
        }
    }, [disconnectWarning, externalApp])

    return (
        <div style={{ width: "100%" }}>
            {alertType === AlertType.AUTHENTICATED && connectedFromSettings && (
                <Alert
                    variant="success"
                    onClose={() => setAlertType("")}
                    dismissible
                    className={styles.alert}
                >
                    Successfully connected! All of your{" "}
                    {externalApp.display_name} files have been synced with your
                    Fractal apps.
                </Alert>
            )}
            {alertType === AlertType.DISCONNECTED && (
                <Alert
                    variant="success"
                    onClose={() => setAlertType("")}
                    dismissible
                    className={styles.alert}
                >
                    Successfully disconnected! Your {externalApp.display_name}{" "}
                    files are no longer synced with your Fractal apps.
                </Alert>
            )}
            {alertType === AlertType.DISCONNECT_WARNING && (
                <Alert
                    variant="danger"
                    onClose={() => setAlertType("")}
                    dismissible
                    className={styles.alert}
                >
                    There was an error disconnecting your{" "}
                    {externalApp.display_name} account. Please try again.
                </Alert>
            )}
        </div>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        apps: {
            authenticated: string
            disconnected: string
            disconnectWarning: string
        }
    }
}) => {
    return {
        authenticated: state.MainReducer.apps.authenticated,
        disconnected: state.MainReducer.apps.disconnected,
        disconnectWarning: state.MainReducer.apps.disconnectWarning,
    }
}

export default connect(mapStateToProps)(CloudStorageAlert)
