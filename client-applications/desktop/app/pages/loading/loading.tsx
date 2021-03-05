import React, { useState, useEffect } from "react"
import { connect } from "react-redux"

import { FractalLogger } from "shared/utils/general/logging"
import { Dispatch } from "shared/types/redux"
import { FractalAuthCache } from "shared/types/cache"
import { validateAccessToken } from "store/actions/auth/sideEffects"
import { updateUser, updateAuthFlow } from "store/actions/auth/pure"
import { updateTask } from "store/actions/container/pure"
import { updateTimer } from "store/actions/client/pure"
import { getComputerInfo } from "store/actions/client/sideEffects"
import { DEFAULT, User, AuthFlow } from "store/reducers/auth/default"
import { deepCopyObject } from "shared/utils/general/reducer"
import { history } from "store/history"
import { FractalRoute } from "shared/types/navigation"
import { FractalIPC } from "shared/types/ipc"
import Animation from "shared/components/loadingAnimation/loadingAnimation"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"
import Version from "shared/components/version"

import styles from "pages/loading/loading.css"

export const Loading = (props: {
    userID: string
    accessToken: string
    encryptionToken: string
    failures: number
    dispatch: Dispatch
}) => {
    /*
        Loading screen that shows up when the app is launched, stays active until the access
        token is either verified or rejected

        Arguments:
            userID: User ID
            accessToken: Access token
            encryptionToken: encryption token for app configs
            failures: Number of login failures
    */

    const { failures, userID, accessToken, encryptionToken, dispatch } = props

    const [listenerCreated, setListenerCreated] = useState(false)
    const [reduxCleared, setReduxCleared] = useState(false)
    const [needsUpdate, setNeedsUpdate] = useState(false)
    const [updateReceived, setUpdateReceived] = useState(false)

    const Store = require("electron-store")
    const storage = new Store()

    const ipc = require("electron").ipcRenderer

    const logger = new FractalLogger()

    useEffect(() => {
        dispatch(updateTimer({ appOpened: Date.now() }))
        dispatch(getComputerInfo())
    }, [dispatch])

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
        if (!updateReceived) {
            return
        }
        logger.logInfo(`Electron-builder update received`, userID)
        if (needsUpdate) {
            logger.logInfo(`Auto update detected`, userID)
            history.push(FractalRoute.UPDATE)
            return
        }
        logger.logInfo(`Auto update NOT detected`, userID)
        if (!reduxCleared) {
            logger.logInfo(`Redux not yet cleared`, userID)
            return
        }
        logger.logInfo(`Redux was cleared successfully`, userID)
        if (userID !== "" && accessToken !== "" && encryptionToken !== "") {
            logger.logInfo(`Authenticated, redirecting to launcher`, userID)
            dispatch(updateTimer({ appDoneLoading: Date.now() }))
            dispatch(updateTask({ shouldLaunchProtocol: true }))
            history.push(FractalRoute.LAUNCHER)
            return
        }
        logger.logInfo(`Not yet authenticated`, userID)
        const cachedAccessToken = storage.get(FractalAuthCache.ACCESS_TOKEN)
        const cachedEncryptionToken = storage.get(FractalAuthCache.ENCRYPTION_TOKEN)
        dispatch(updateUser({encryptionToken: cachedEncryptionToken}))
        // If we have the cached access token and encryption token, and have not unsuccessfully verified them,
        // then verify the cached access token. Otherwise, redirect the user to login.
        const shouldValidate = !!(cachedAccessToken && cachedEncryptionToken && failures === 0)
        if (shouldValidate) {
            logger.logInfo(`Dispatching validate access token`, userID)
            dispatch(validateAccessToken(cachedAccessToken))
        } else {
            logger.logInfo(
                `Redirecting to login, cached access token is ${cachedAccessToken}, cached encryption token is ${cachedEncryptionToken}, and validation failures is ${failures.toString()}`,
                userID
            )
            dispatch(updateTimer({ appDoneLoading: Date.now() }))
            history.push(FractalRoute.LOGIN)
        }
    }, [
        reduxCleared,
        accessToken,
        encryptionToken,
        updateReceived,
        needsUpdate,
        failures,
        dispatch,
    ])

    return (
        <div className={styles.loadingWrapper}>
            <ChromeBackground />
            <div className={styles.loadingCentered}>
                <Animation />
                <div className={styles.loadingText}>Loading</div>
            </div>
            <div>
                <Version />
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
        encryptionToken: state.AuthReducer.user.encryptionToken,
        failures: state.AuthReducer.authFlow.failures,
    }
}

export default connect(mapStateToProps)(Loading)
