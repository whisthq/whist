import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import { AvailableLoggers } from "shared/types/logs"
import { FractalLogger } from "shared/utils/general/logging"
import { Dispatch } from "shared/types/redux"
import { FractalAuthCache } from "shared/types/cache"
import { validateAccessToken } from "store/actions/auth/sideEffects"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import { DEFAULT, User, AuthFlow } from "store/reducers/auth/default"
import { deepCopyObject } from "shared/utils/general/reducer"
import { history } from "store/history"
import { FractalRoute } from "shared/types/navigation"
import { FractalIPC } from "shared/types/ipc"
import Animation from "shared/components/loadingAnimation/loadingAnimation"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"

import styles from "pages/loading/loading.css"

export const Loading = (props: {
    userID: string
    accessToken: string
    failures: number
    dispatch: Dispatch
}) => {
    /*
        Loading screen that shows up when the app is launched, stays active until the access
        token is either verified or rejected
 
        Arguments:
            userID: User ID
            accessToken: Access token  
            failures: Number of login failures
    */

    const { failures, userID, accessToken, dispatch } = props

    const [listenerCreated, setListenerCreated] = useState(false)
    const [reduxCleared, setReduxCleared] = useState(false)
    const [needsUpdate, setNeedsUpdate] = useState(false)
    const [updateReceived, setUpdateReceived] = useState(false)

    const Store = require("electron-store")
    const storage = new Store()

    const ipc = require("electron").ipcRenderer

    const logger = new FractalLogger(AvailableLoggers.LOADING)

    useEffect(() => {
        if (!listenerCreated) {
            setListenerCreated(true)
            ipc.on(FractalIPC.UPDATE, (_: any, update: boolean) => {
                logger.logInfo(
                    `IPC update received, needs update is ${false}`,
                    userID
                )
                setNeedsUpdate(update)
                setUpdateReceived(true)
            })
        }
    }, [listenerCreated])

    // On app launch, clear the Redux state so we can re-check auth credentials
    useEffect(() => {
        logger.logInfo(`---------------- LAUNCHED ------------------`, userID)

        dispatch(updateUser(deepCopyObject(DEFAULT.user)))
        dispatch(updateAuthFlow(deepCopyObject(DEFAULT.authFlow)))
        setReduxCleared(true)
        logger.logInfo(`User and auth flow reset cleared`, userID)
    }, [dispatch])

    // Check for update. If update available, then update, otherwise proceed to authenticate user
    useEffect(() => {
        // First, check to see if the autoupdater checked for an update
        switch (updateReceived) {
            case true:
                logger.logInfo(`Electron-builder update received`, userID)
                switch (needsUpdate) {
                    // If an update is needed, update
                    case true:
                        logger.logInfo(`Auto update detected`, userID)
                        history.push(FractalRoute.UPDATE)
                        break
                    // If an update is not needed, check to see if we have cleared the Redux state already
                    case false:
                        logger.logInfo(`Auto update NOT detected`, userID)
                        switch (reduxCleared) {
                            // If we have, check to see if we have login credentials
                            case true:
                                logger.logInfo(
                                    `Redux was cleared successfully`,
                                    userID
                                )
                                switch (userID !== "" && accessToken !== "") {
                                    // If we do, skip the login screen
                                    case true:
                                        logger.logInfo(
                                            `Authenticated, redirecting to launcher`,
                                            userID
                                        )
                                        history.push(FractalRoute.LAUNCHER)
                                        break
                                    // If we don't, check to see if we have login credentials cached
                                    case false: {
                                        logger.logInfo(
                                            `Not yet authenticated`,
                                            userID
                                        )
                                        const cachedAccessToken = storage.get(
                                            FractalAuthCache.ACCESS_TOKEN
                                        )
                                        // If we have the cached access token and have not unsuccessfully verified them,
                                        // then verify the cahced access token. Otherwise, redirect the user to login.
                                        const shouldValidate = !!(
                                            cachedAccessToken && failures === 0
                                        )

                                        switch (shouldValidate) {
                                            case true:
                                                logger.logInfo(
                                                    `Dispatching validate access token`,
                                                    userID
                                                )
                                                dispatch(
                                                    validateAccessToken(
                                                        cachedAccessToken
                                                    )
                                                )
                                                break
                                            case false:
                                                logger.logInfo(
                                                    `Redirecting to login, cached access token is ${cachedAccessToken} and validation failures is ${failures.toString()}`,
                                                    userID
                                                )
                                                history.push(FractalRoute.LOGIN)
                                                break
                                            default:
                                                break
                                        }
                                        break
                                    }
                                    default:
                                        break
                                }
                                break
                            case false:
                                logger.logInfo(`Redux not yet cleared`, userID)
                                break
                            default:
                                break
                        }
                        break
                    default:
                        break
                }
                break
            case false:
                break
            default:
                break
        }
    }, [reduxCleared, accessToken, updateReceived, needsUpdate, failures])

    return (
        <div className={styles.loadingWrapper}>
            <ChromeBackground />
            <div className={styles.loadingCentered}>
                <Animation />
                <div className={styles.loadingText}>Loading</div>
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        userID: state.AuthReducer.user.userID,
        accessToken: state.AuthReducer.user.accessToken,
        failures: state.AuthReducer.authFlow.failures,
    }
}

export default connect(mapStateToProps)(Loading)
