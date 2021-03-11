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

import { updateUser } from "store/actions/auth/pure"
import { updateContainer, updateTask } from "store/actions/container/pure"
import { updateTimer } from "store/actions/client/pure"
import {
    getContainerInfo,
    createContainer,
} from "store/actions/container/sideEffects"
import { FractalIPC } from "shared/types/ipc"
import { BROWSER_WINDOW_IDS } from "shared/types/browsers"
import { AWSRegion } from "shared/types/aws"
import { FractalAuthCache } from "shared/types/cache"
import { FractalDirectory } from "shared/types/client"
import { uploadToS3 } from "shared/utils/files/aws"

import { launchProtocol, writeStream } from "shared/utils/files/exec"
import Animation from "shared/components/loadingAnimation/loadingAnimation"
import LoadingMessage from "pages/launcher/constants/loadingMessages"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"

import styles from "pages/launcher/launcher.css"
import { ChildProcess } from "child_process"

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
    configKey: string
}) => {
    /*
        Protocol launcher with animated loading screen. User will be redirected to this page
        if they are successfully logged in.

        Arguments:
            userID (string): User ID
            taskID (string): Container creation celery ID
            container (Container): Container from Redux state
            dispatch (Dispatch): Redux action dispatcher
            configKey (string): configuration key to encrypt app configs
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
        configKey,
    } = props

    const ipc = require("electron").ipcRenderer
    const logger = new FractalLogger()
    const Store = require("electron-store")
    const storage = new Store()

    const [taskState, setTaskState] = useState(FractalAppState.NO_TASK)
    const [protocol, setProtocol] = useState<ChildProcess>()
    const [protocolLock, setProtocolLock] = useState(false)
    const [disconnected, setDisconnected] = useState(false)
    const [shouldForceQuit, setShouldForceQuit] = useState(false)
    const [timedOut, setTimedOut] = useState(false)
    const [killSignalsReceived, setKillSignalsReceived] = useState(0)
    const [loadingMessage, setLoadingMessage] = useState("")

    const [loginClosed, setLoginClosed] = useState(false)
    const [configKeyRetrieved, setConfigKeyRetrieved] = useState(false)

    const { data, loading, error } = useSubscription(SUBSCRIBE_USER_APP_STATE, {
        variables: { taskID: taskID },
    })

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

    // Callback function meant to be fired before protocol starts
    const protocolOnStart = () => {
        // IPC sends boolean to the main thread to hide the Electron browser Window
        logger.logInfo("Protocol started, callback fired", userID)
        dispatch(updateTimer({ protocolLaunched: Date.now() }))
    }

    // Callback function meant to be fired when protocol exits
    const protocolOnExit = () => {
        // Log timer analytics data
        dispatch(updateTimer({ protocolClosed: Date.now() }))

        // For S3 protocol client log upload
        const logPath = require("path").join(
            FractalDirectory.getRootDirectory(),
            require("os").platform() === "darwin"
                ? "MacOS/log.txt"
                : "protocol-build/desktop/log.txt"
        )

        const s3FileName = `CLIENT_${userID}_${new Date().getTime()}.txt`

        logger.logInfo(
            `Protocol client logs: https://fractal-protocol-logs.s3.amazonaws.com/${s3FileName}`,
            userID
        )
        // Clear the Redux state just in case
        resetReduxforLaunch()

        logger.logInfo(`Should force quit is ${shouldForceQuit}`, userID)

        // Upload client logs to S3 and shut down Electron
        uploadToS3(logPath, s3FileName, (s3Error: string) => {
            if (s3Error) {
                logger.logError(`Upload to S3 errored: ${s3Error}`, userID)
            }
            setDisconnected(true)
        })
    }

    useEffect(() => {
        setTimeout(() => {
            setTimedOut(true)
        }, TIMEOUT)
    }, [])

    useEffect(() => {
        if (!configKeyRetrieved) {
            if (
                ipc.sendSync(FractalIPC.CHECK_BROWSER, [
                    BROWSER_WINDOW_IDS.LOGIN,
                ])
            ) {
                const retrievedConfigKey = ipc.sendSync(FractalIPC.GET_CONFIG_KEY)
                dispatch(updateUser({ configKey: retrievedConfigKey }))
                storage.set(FractalAuthCache.CONFIG_KEY, retrievedConfigKey)
            }
            setConfigKeyRetrieved(true)
        } else if (!configKey || configKey === "") {
            setTaskState(FractalAppState.FAILURE)
            logger.logError("User missing configuration key", userID)
        }
    }, [configKeyRetrieved])

    useEffect(() => {
        if (!loginClosed && configKeyRetrieved) {
            ipc.sendSync(FractalIPC.CLOSE_BROWSER, [BROWSER_WINDOW_IDS.LOGIN])
            setLoginClosed(true)
        }
    }, [loginClosed, configKeyRetrieved])

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
                writeStream(protocol, "kill?0")
                protocol.kill("SIGINT")
                setProtocolLock(false)
                setProtocol(undefined)
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

            const launchProtocolAsync = async () => {
                const childProcess = await launchProtocol(
                    protocolOnStart,
                    protocolOnExit
                )
                setProtocol(childProcess)
                setShouldForceQuit(true)
            }

            launchProtocolAsync()

            logger.logInfo("Dispatching create container action", userID)
            dispatch(updateTimer({ createContainerRequestSent: Date.now() }))
        }
    }, [dispatch, protocol, shouldLaunchProtocol, protocolLock])

    useEffect(() => {
        if (
            protocol &&
            taskState === FractalAppState.NO_TASK &&
            region &&
            configKeyRetrieved
        ) {
            setTaskState(FractalAppState.PENDING)
            dispatch(createContainer())
        }
    }, [protocol, region, taskState, configKeyRetrieved])

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
                        writeStream(
                            protocol,
                            `loading?${LoadingMessage.PENDING}`
                        )
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
            const portInfo = `32262:${container.port32262}.32263:${container.port32263}.32273:${container.port32273}`
            writeStream(protocol, `ports?${portInfo}`)
            writeStream(protocol, `private-key?${container.secretKey}`)
            writeStream(protocol, `ip?${container.publicIP}`)
            writeStream(protocol, `finished?0`)
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
        configKey: state.AuthReducer.user.configKey,
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
