import React from "react"
import { connect } from "react-redux"
import { FaCheck } from "react-icons/fa"

import { config } from "shared/constants/config"
import { openExternal } from "shared/utils/general/helpers"
import { ExternalApp } from "store/reducers/types"

import styles from "pages/onboard/onboard.css"

const StorageService = (props: {
    externalApp: ExternalApp
    connected: boolean
    accessToken: string
}) => {
    const { externalApp, connected, accessToken } = props

    const handleConnect = () => {
        openExternal(
            `${config.url.WEBSERVER_URL}/oauth/authorize?external_app=${externalApp.code_name}&access_token=${accessToken}`
        )
    }

    return (
        <div className={styles.storageServiceContainer}>
            <img
                alt="Service logo"
                src={externalApp.image_s3_uri}
                className={styles.serviceLogo}
            />
            <div className={styles.displayName}>{externalApp.display_name}</div>
            {connected ? (
                <FaCheck className={styles.faCheck} />
            ) : (
                <button
                    type="button"
                    className={styles.connectButton}
                    onClick={handleConnect}
                >
                    CONNECT
                </button>
            )}
        </div>
    )
}

export const mapStateToProps = (state: {
    MainReducer: {
        auth: {
            accessToken: string
        }
    }
}) => {
    return {
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(StorageService)
