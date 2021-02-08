import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { useSubscription } from "@apollo/client"

// import { FadeIn } from "react-fade-in"

import { FractalLogger } from "shared/utils/general/logging"
import { Dispatch } from "shared/types/redux"
import { SUBSCRIBE_USER_APP_STATE } from "shared/constants/graphql"
import { FractalAppState, FractalTaskStatus } from "shared/types/containers"
import { deepCopyObject } from "shared/utils/general/reducer"
import { User } from "store/reducers/auth/default"
import {
    Timer,
    DEFAULT as AnalyticsDefault,
} from "store/reducers/analytics/default"
import {
    Container,
    Task,
    DEFAULT as ContainerDefault,
} from "store/reducers/container/default"

import { updateContainer, updateTask } from "store/actions/container/pure"
import { updateTimer } from "store/actions/analytics/pure"
import {
    getContainerInfo,
    createContainer,
} from "store/actions/container/sideEffects"
import { FractalIPC } from "shared/types/ipc"
import { FractalDirectory } from "shared/types/client"
import { uploadToS3 } from "shared/utils/files/aws"

import { launchProtocol, writeStream, endStream } from "shared/utils/files/exec"
import Animation from "shared/components/loadingAnimation/loadingAnimation"
import LoadingMessage from "pages/launcher/constants/loadingMessages"
import ChromeBackground from "shared/components/chromeBackground/chromeBackground"

import styles from "pages/launcher/launcher.css"
import { ChildProcess } from "child_process"

export const Launcher = (props: {
    userID: string
    protocol: ChildProcess | undefined
    taskID: string
    status: FractalTaskStatus
    shouldLaunchProtocol: boolean
    container: Container
    timer: Timer
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
        protocol,
        taskID,
        status,
        shouldLaunchProtocol,
        container,
        timer,
        dispatch,
    } = props

    const [taskState, setTaskState] = useState(FractalAppState.NO_TASK)

    const { data, loading, error } = useSubscription(SUBSCRIBE_USER_APP_STATE, {
        variables: { taskID: taskID },
    })

    const ipc = require("electron").ipcRenderer
    const logger = new FractalLogger()

    const startTimeout = () => {
        setTimeout(() => {
            if (!container.containerID && taskState !== FractalAppState.READY) {
                setTaskState(FractalAppState.FAILURE)
                logger.logError("Container took too long to create", userID)
            }
        }, 60000)
    }
    // Restores Redux state to before a container was created
    const resetLaunch = () => {
        const defaultContainerState = deepCopyObject(ContainerDefault.container)
        const defaultTimerState = deepCopyObject(AnalyticsDefault.timer)
        const defaultTaskState = deepCopyObject(ContainerDefault.task)

        // Erase old container info
        dispatch(updateContainer(defaultContainerState))
        // Erase old task info
        dispatch(updateTask(defaultTaskState))
        // Erase old timer info
        dispatch(updateTimer(defaultTimerState))

        startTimeout()
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

        setTaskState(FractalAppState.NO_TASK)
        resetLaunch()
        dispatch(updateTask({ shouldLaunchProtocol: true }))
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
            "protocol-build/desktop/log.txt"
        )

        const s3FileName = `CLIENT_${userID}_${new Date().getTime()}.txt`

        logger.logInfo(
            `Protocol client logs: https://fractal-protocol-logs.s3.amazonaws.com/${s3FileName}`,
            userID
        )
        // Clear the Redux state just in case
        resetLaunch()

        // Upload client logs to S3 and shut down Electron
        uploadToS3(logPath, s3FileName, (s3Error: string) => {
            if (s3Error) {
                logger.logError(`Upload to S3 errored: ${s3Error}`, userID)
            }
            if (container && container.containerID) {
                forceQuit()
            }
        })
    }

    // Set timeout for maximum time before we stop waiting for container/assing
    useEffect(() => {
        startTimeout()
    }, [])

    useEffect(() => {
        console.log(taskState)
        if (taskState === FractalAppState.FAILURE) {
            if (protocol) {
                protocol.kill("SIGINT")
            }
            ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, true)
        }
    }, [taskState])

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
        if (!protocol && shouldLaunchProtocol) {
            // ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, false)

            const launchProtocolAsync = async () => {
                const childProcess = await launchProtocol(
                    protocolOnStart,
                    protocolOnExit
                )
                dispatch(
                    updateTask({
                        childProcess: childProcess,
                        shouldLaunchProtocol: false,
                    })
                )
            }

            launchProtocolAsync()

            logger.logInfo("Dispatching create container action", userID)
            dispatch(updateTimer({ createContainerRequestSent: Date.now() }))
        }
    }, [dispatch, protocol, shouldLaunchProtocol])

    useEffect(() => {
        if (protocol && taskState === FractalAppState.NO_TASK) {
            writeStream(protocol, `loading?CREATING_CONTAINER`)
            setTaskState(FractalAppState.PENDING)
            dispatch(createContainer())
        }
    }, [protocol])

    // Listen to container creation task state
    useEffect(() => {
        writeStream(protocol, "loading?USE_EFFECT")

        if (error) {
            logger.logError(
                `User container subscription errored: ${error}`,
                userID
            )
            setTaskState(FractalAppState.FAILURE)
        } else {
            const currentState =
                data &&
                data.hardware_user_app_state &&
                data.hardware_user_app_state[0]
                    ? data.hardware_user_app_state[0].state
                    : null

            logger.logInfo(`User container state: ${currentState}`, userID)

            if (currentState && taskState !== FractalAppState.FAILURE) {
                setTaskState(currentState)
                switch (currentState) {
                    case FractalAppState.PENDING:
                        writeStream(
                            protocol,
                            `loading?${LoadingMessage.PENDING}`
                        )
                        break
                    case FractalAppState.READY:
                        dispatch(getContainerInfo(taskID))
                        break
                    case FractalAppState.FAILURE:
                        logger.logError(
                            "Container creation state is FAILURE",
                            userID
                        )
                        setTaskState(FractalAppState.FAILURE)
                        writeStream(
                            protocol,
                            `loading?${LoadingMessage.FAILURE}`
                        )
                        break
                    default:
                        break
                }
            }
        }
    }, [data, loading, error])

    // If container has been created and protocol hasn't been launched yet, launch protocol
    useEffect(() => {
        if (container && container.containerID && protocol) {
            logger.logInfo(
                `Received container IP ${container.publicIP}, piping to protocol`,
                userID
            )
            const portInfo = `32262:${container.port32262}.32263:${container.port32263}.32273:${container.port32273}`
            writeStream(protocol, `ports?${portInfo}`)
            const streamWritten = writeStream(
                protocol,
                `private-key?${container.secretKey}`
            )
            logger.logInfo(
                `Stream write success is ${streamWritten.toString()}`,
                userID
            )
            const streamEnded = endStream(protocol, `ip?${container.publicIP}`)
            logger.logInfo(
                `Stream end success is ${streamEnded.toString()}`,
                userID
            )
        }
    }, [container, protocol])

    return (
        <div className={styles.launcherWrapper}>
            <ChromeBackground />
            <div className={styles.loadingWrapper}>
                <Animation />
                {taskState === FractalAppState.FAILURE && (
                    <div style={{ marginTop: 35 }}>
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
    AnalyticsReducer: {
        timer: Timer
    }
}) => {
    return {
        userID: state.AuthReducer.user.userID,
        protocol: state.ContainerReducer.task.childProcess,
        taskID: state.ContainerReducer.task.taskID,
        status: state.ContainerReducer.task.status,
        shouldLaunchProtocol: state.ContainerReducer.task.shouldLaunchProtocol,
        container: state.ContainerReducer.container,
        timer: state.AnalyticsReducer.timer,
    }
}

export default connect(mapStateToProps)(Launcher)
