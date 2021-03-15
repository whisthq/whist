import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { useSubscription } from "@apollo/client"

import { FractalLogger } from "shared/utils/general/logging"
import { Dispatch } from "shared/types/redux"
import { SUBSCRIBE_USER_APP_STATE } from "shared/constants/graphql"
import { FractalAppState, FractalTaskStatus } from "shared/types/containers"
import { deepCopyObject } from "shared/utils/general/reducer"
import { User } from "store/reducers/auth/default"
import {
    Timer,
    ComputerInfo,
    DEFAULT as ClientDefault,
} from "store/reducers/client/default"
import {
    Container,
    Task,
    DEFAULT as ContainerDefault,
} from "store/reducers/container/default"

import { updateContainer, updateTask } from "store/actions/container/pure"
import { updateTimer } from "store/actions/client/pure"
import {
    getContainerInfo,
    createContainer,
} from "store/actions/container/sideEffects"
import { FractalIPC } from "shared/types/ipc"
import { AWSRegion } from "shared/types/aws"
import { FractalDirectory } from "shared/types/client"
import { uploadToS3 } from "shared/utils/files/aws"

import Animation from "shared/components/loadingAnimation/loadingAnimation"
import LoadingMessage from "pages/launcher/constants/loadingMessages"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"

import styles from "pages/launcher/launcher.css"

/*
    Amount of time passed before giving up on container/assign
    60000 = 1 minute
*/
const SECOND = 1000
const TIMEOUT = 60 * SECOND

export const Launcher = (props: {
    userID: string
    taskID: string
    status: FractalTaskStatus
    shouldLaunchProtocol: boolean
    protocolKillSignal: number
    container: Container
    timer: Timer
    region: AWSRegion | undefined
    dispatch: Dispatch
}) => {
    /*
        Protocol launcher with animated loading screen. User will be redirected to this page
        if they are successfully logged in.

        Arguments:
            userID (string): User ID
            taskID (string): Container creation celery ID
            container (Container): Container from Redux state
    */

    const {
        userID,
        taskID,
        status,
        shouldLaunchProtocol,
        protocolKillSignal,
        container,
        timer,
        region,
        dispatch,
    } = props

    const [taskState, setTaskState] = useState(FractalAppState.NO_TASK)
    const [protocol, setProtocol] = useState(false)
    const [protocolLock, setProtocolLock] = useState(false)
    const [disconnected, setDisconnected] = useState(false)
    const [shouldForceQuit, setShouldForceQuit] = useState(false)
    const [timedOut, setTimedOut] = useState(false)
    const [killSignalsReceived, setKillSignalsReceived] = useState(0)
    const [loadingMessage, setLoadingMessage] = useState("")

    const [loginClosed, setLoginClosed] = useState(false)

    const { data, loading, error } = useSubscription(SUBSCRIBE_USER_APP_STATE, {
        variables: { taskID: taskID },
    })

    const ipc = require("electron").ipcRenderer
    const logger = new FractalLogger()

    // Restores Redux state to before a container was created
    const resetReduxforLaunch = () => {
        const defaultContainerState = deepCopyObject(ContainerDefault.container)
        const defaultTimerState = deepCopyObject(ClientDefault.timer)
        const defaultTaskState = deepCopyObject(ContainerDefault.task)

        // Erase old container info
        dispatch(updateContainer(defaultContainerState))
        // Erase old task info
        dispatch(updateTask(defaultTaskState))
        // Erase old timer info
        dispatch(updateTimer(defaultTimerState))
    }

    // If the webserver failed, function to try again
    const tryAgain = () => {
        if (taskState === FractalAppState.FAILURE) {
            logger.logError(
                "App state is failure, try again button pressed",
                userID
            )
        } else if (status === FractalTaskStatus.FAILURE) {
            logger.logError(
                "Celery task is failure, try again button pressed",
                userID
            )
        }

        // No more celery task, reset to NO_TASK
        setTaskState(FractalAppState.NO_TASK)
        // Clear Redux
        resetReduxforLaunch()
        dispatch(updateTask({ shouldLaunchProtocol: true }))
        // Remove lock so protocol can be relaunched
        setProtocolLock(false)
        // Reset timeout
        setTimedOut(false)

        setTimeout(() => {
            setTimedOut(true)
        }, TIMEOUT)
    }

    const forceQuit = () => {
        setTimeout(() => {
            ipc.sendSync(FractalIPC.FORCE_QUIT)
        }, 1000)
    }

    useEffect(() => {
        setTimeout(() => {
            setTimedOut(true)
        }, TIMEOUT)
    }, [])

    useEffect(() => {
        if (!loginClosed) {
            ipc.sendSync(FractalIPC.CLOSE_OTHER_WINDOWS)
            setLoginClosed(true)
        }
    }, [loginClosed])

    useEffect(() => {
        if (userID) {
            ipc.sendSync(FractalIPC.SET_USERID, userID)
        }
    }, [userID])

    useEffect(() => {
        if (timedOut) {
            setLoadingMessage("Error: It took too long to open your browser.")
            logger.logInfo(JSON.stringify(container), userID)

            if (!container.containerID && taskState !== FractalAppState.READY) {
                setTaskState(FractalAppState.FAILURE)
                logger.logError("Container took too long to create", userID)
            }
        } else {
            setLoadingMessage("")
        }
    }, [timedOut])

    useEffect(() => {
        if (shouldForceQuit && disconnected) {
            forceQuit()
        }
    }, [shouldForceQuit, disconnected])

    useEffect(() => {
        if (
            taskState === FractalAppState.FAILURE ||
            protocolKillSignal > killSignalsReceived
        ) {
            // If the task failed or we manually kill the protocol due to a login
            // or payment issue, don't quit the entire Electron app
            setShouldForceQuit(false)

            logger.logError(
                "Task state is FAILURE, sending protocol kill",
                userID
            )
            if (protocol) {
                ipc.sendSync(FractalIPC.KILL_PROTOCOL, true)
                setProtocolLock(false)
                setProtocol(false)
            }
            ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, true)
            setKillSignalsReceived(protocolKillSignal)
        }
    }, [taskState, protocolKillSignal])

    // Log timer analytics
    useEffect(() => {
        if (timer.protocolClosed && timer.protocolClosed > 0) {
            logger.logInfo(
                `Protocol exited! Timer logs are ${JSON.stringify(timer)}`,
                userID
            )
        }
    }, [timer])

    // On app start, check if a container creation request has been sent.
    // If not, create a container
    useEffect(() => {
        if (!protocol && shouldLaunchProtocol && !protocolLock) {
            setProtocolLock(true)
            dispatch(
                updateTask({
                    shouldLaunchProtocol: false,
                })
            )

            ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, false)
            ipc.sendSync(FractalIPC.LAUNCH_PROTOCOL, true)
            setShouldForceQuit(true)
            setProtocol(true)

            logger.logInfo("Dispatching create container action", userID)
            dispatch(updateTimer({ createContainerRequestSent: Date.now() }))
        }
    }, [dispatch, protocol, shouldLaunchProtocol, protocolLock])

    useEffect(() => {
        if (protocol && taskState === FractalAppState.NO_TASK && region) {
            setTaskState(FractalAppState.PENDING)
            dispatch(createContainer())
        }
    }, [protocol, region, taskState])

    // Listen to container creation task state
    useEffect(() => {
        if (error) {
            logger.logError(
                `User container subscription errored: ${error}`,
                userID
            )
            setTaskState(FractalAppState.FAILURE)
        } else if (!timedOut) {
            const currentState =
                data &&
                data.hardware_user_app_state &&
                data.hardware_user_app_state[0]
                    ? data.hardware_user_app_state[0].state
                    : null

            logger.logInfo(`User container state: ${currentState}`, userID)

            if (currentState) {
                setTaskState(currentState)
                switch (currentState) {
                    case FractalAppState.PENDING:
                        ipc.sendSync(FractalIPC.PENDING_PROTOCOL, true)
                        break
                    case FractalAppState.SPINNING_UP_NEW:
                        setLoadingMessage(LoadingMessage.PENDING)
                        break
                    case FractalAppState.READY:
                        dispatch(getContainerInfo(taskID))
                        break
                    case FractalAppState.FAILURE:
                        logger.logError(
                            "Container creation state is FAILURE",
                            userID
                        )
                        setLoadingMessage("Oops! Unexpected server error.")
                        setTaskState(FractalAppState.FAILURE)
                        break
                    default:
                        break
                }
            }
        }
    }, [data, loading, error, timedOut])

    // If container has been created and protocol hasn't been launched yet, launch protocol
    useEffect(() => {
        if (container && container.containerID && protocol) {
            logger.logInfo(
                `Received container IP ${container.publicIP}, piping to protocol`,
                userID
            )
            ipc.sendSync(FractalIPC.SEND_CONTAINER, container)
        }
    }, [container, protocol])

    return (
        <div className={styles.launcherWrapper}>
            <ChromeBackground />
            <div className={styles.loadingWrapper}>
                <Animation />
                {taskState === FractalAppState.FAILURE && (
                    <div style={{ marginTop: 35 }}>
                        <div style={{ marginBottom: 25 }}>{loadingMessage}</div>
                        <button
                            type="button"
                            className={styles.greenButton}
                            onClick={tryAgain}
                        >
                            Try Again
                        </button>
                    </div>
                )}
            </div>
        </div>
    )
}

export const mapStateToProps = (state: {
    AuthReducer: {
        user: User
    }
    ContainerReducer: {
        task: Task
        container: Container
    }
    ClientReducer: {
        timer: Timer
        computerInfo: ComputerInfo
    }
}) => {
    return {
        userID: state.AuthReducer.user.userID,
        taskID: state.ContainerReducer.task.taskID,
        status: state.ContainerReducer.task.status,
        shouldLaunchProtocol: state.ContainerReducer.task.shouldLaunchProtocol,
        protocolKillSignal: state.ContainerReducer.task.protocolKillSignal,
        container: state.ContainerReducer.container,
        region: state.ClientReducer.computerInfo.region,
        timer: state.ClientReducer.timer,
    }
}

export default connect(mapStateToProps)(Launcher)
