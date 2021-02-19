import React, { useEffect } from "react"
import { connect } from "react-redux"

import { Dispatch } from "shared/types/redux"
import { User, DEFAULT as AuthDefault } from "store/reducers/auth/default"
import { FractalLogger } from "shared/utils/general/logging"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"
import { openExternal } from "shared/utils/general/helpers"
import { config } from "shared/constants/config"
import { FractalRoute } from "shared/types/navigation"
import { history } from "store/history"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import { FractalAuthCache } from "shared/types/cache"
import { deepCopyObject } from "shared/utils/general/reducer"
import { FractalIPC } from "shared/types/ipc"
import { updateTask } from "store/actions/container/pure"

import styles from "pages/payment/payment.css"

export const Payment = (props: { userID: string; dispatch: Dispatch }) => {
    /*
        Loading screen that shows up when the app is launched, stays active until the access
        token is either verified or rejected
 
        Arguments:
            userID: User ID
            accessToken: Access token  
            failures: Number of login failures
    */

    const { userID, dispatch } = props
    const logger = new FractalLogger()
    const Store = require("electron-store")
    const storage = new Store()
    const ipc = require("electron").ipcRenderer

    const upgrade = () => {
        openExternal(`${config.url.FRONTEND_URL}/dashboard/settings/payment`)
    }

    const refresh = () => {
        dispatch(updateTask({ shouldLaunchProtocol: true }))
        history.push(FractalRoute.LAUNCHER)
    }

    const logout = () => {
        dispatch(updateUser(deepCopyObject(AuthDefault.user)))
        dispatch(updateAuthFlow(deepCopyObject(AuthDefault.authFlow)))
        storage.set(FractalAuthCache.ACCESS_TOKEN, null)
        history.push(FractalRoute.LOGIN)
    }

    useEffect(() => {
        logger.logInfo("Payment required page loaded", userID)
        ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, true)
    }, [])

    return (
        <div className={styles.paymentWrapper}>
            <ChromeBackground />
            <button
                type="button"
                onClick={logout}
                className={styles.logoutButton}
            >
                Logout
            </button>
            <div className={styles.paymentCentered}>
                <div style={{ fontSize: 60 }}>Your free trial has expired.</div>
                <div className={styles.paymentText}>
                    Please upgrade your plan to continue using Fractal. Once you
                    have upgraded, refresh this page.
                </div>
                <div style={{ display: "flex" }}>
                    <button
                        type="button"
                        className={styles.transparentButton}
                        onClick={upgrade}
                    >
                        Upgrade
                    </button>
                    <button
                        type="button"
                        className={styles.transparentButton}
                        onClick={refresh}
                        style={{
                            marginLeft: 25,
                            background: "none",
                            border: "solid 1px black",
                            color: "black",
                        }}
                    >
                        Refresh Page
                    </button>
                </div>
            </div>
        </div>
    )
}

const mapStateToProps = (state: { AuthReducer: { user: User } }) => {
    return {
        userID: state.AuthReducer.user.userID,
    }
}

export default connect(mapStateToProps)(Payment)
