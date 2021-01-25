import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { useSubscription } from "@apollo/client"
import S3 from "aws-s3"

// import { FadeIn } from "react-fade-in"

import { FractalLogger } from "shared/utils/general/logging"
import { AvailableLoggers } from "shared/types/logs"
import { Dispatch } from "shared/types/redux"
import { SUBSCRIBE_USER_APP_STATE } from "shared/constants/graphql"
import { FractalAppState, FractalTaskStatus } from "shared/types/containers"
import { deepCopyObject } from "shared/utils/general/reducer"
import { User } from "store/reducers/auth/default"
import { Container, Task, DEFAULT } from "store/reducers/container/default"
import { updateContainer, updateTask } from "store/actions/container/pure"
import {
    getContainerInfo,
    createContainer,
} from "store/actions/container/sideEffects"
import { FractalIPC } from "shared/types/ipc"
import { FractalDirectory } from "shared/types/client"
import { uploadToS3 } from "shared/utils/files/aws"

import { launchProtocol } from "shared/utils/files/exec"
import Animation from "shared/components/loadingAnimation/loadingAnimation"
import LoadingMessage from "pages/launcher/constants/loadingMessages"

import styles from "pages/launcher/launcher.css"

export const Launcher = (props: {
    userID: string
    running: boolean
    taskID: string
    status: FractalTaskStatus
    container: Container
    dispatch: Dispatch
}) => {
    /*
        Protocol launcher with animated loading screen. User will be redirected to this page
        if they are successfully logged in.
 
        Arguments:
            userID (string): User ID
            running (boolean): true if a container has been created or protocol is running
            taskID (string): Container creation celery ID
            container (Container): Container from Redux state
    */

    const { userID, running, taskID, status, container, dispatch } = props

    const [protocolLaunched, setProtocolLaunched] = useState(false)
    const [loadingMessage, setLoadingMessage] = useState(
        LoadingMessage.STARTING
    )
    const [taskState, setTaskState] = useState(FractalAppState.PENDING)

    const { data, loading, error } = useSubscription(SUBSCRIBE_USER_APP_STATE, {
        variables: { taskID: taskID },
    })

    const ipc = require("electron").ipcRenderer

    const logger = new FractalLogger(AvailableLoggers.LOADING)

    // Restores Redux state to before a container was created
    const resetLaunch = () => {
        const defaultContainerState = deepCopyObject(DEFAULT.container)
        const defaultTaskState = deepCopyObject(DEFAULT.task)

        // Erase old container info
        dispatch(updateContainer(defaultContainerState))
        // Erase old task info
        dispatch(updateTask(defaultTaskState))
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
        setLoadingMessage(LoadingMessage.STARTING)
        resetLaunch()
    }

    // Callback function meant to be fired before protocol starts
    const protocolOnStart = () => {
        // IPC sends boolean to the main thread to hide the Electron browser Window
        logger.logInfo("Protocol started, callback fired", userID)
        // ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW, false)
    }

    // Callback function meant to be fired when protocol exits
    const protocolOnExit = () => {
        resetLaunch()

        const logPath = require("path").join(
            FractalDirectory.getRootDirectory(),
            "protocol-build/desktop/log.txt"
        )

        const s3FileName = `CLIENT${new Date().getTime()}.txt`

        uploadToS3(logPath, s3FileName, (error: string) => {
            if (error) {
                logger.logInfo(`Upload to S3 errored: ${error}`, userID)
            } else {
                logger.logInfo(
                    `Protocol client logs: https://fractal-protocol-logs.s3.amazonaws.com/${s3FileName}`,
                    userID
                )
            }
            // Quit the app
            logger.logInfo("------------- EXITED -----------------", userID)
            ipc.sendSync(FractalIPC.FORCE_QUIT)
        })
    }

    // On app start, check if a container creation request has been sent.
    // If not, create a container and set running = true to prevent multiple containers
    // from being created.
    useEffect(() => {
        if (!running && !protocolLaunched) {
            logger.logInfo("Dispatching create container action", userID)
            dispatch(updateTask({ running: true }))
            dispatch(createContainer())
        }
    }, [running])

    // Listen to container creation task state
    useEffect(() => {
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

            if (currentState) {
                setTaskState(currentState)
                switch (currentState) {
                    case FractalAppState.PENDING:
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
                        setLoadingMessage(LoadingMessage.FAILURE)
                        break
                    default:
                        break
                }
            }
        }
    }, [data, loading, error])

    // If container has been created and protocol hasn't been launched yet, launch protocol
    useEffect(() => {
        if (container.containerID && !protocolLaunched) {
            logger.logInfo(
                `Container ${JSON.stringify(
                    container
                )} detected, launching protocol`,
                userID
            )

            setProtocolLaunched(true)
            launchProtocol(container, protocolOnStart, protocolOnExit)
        }
    }, [container, protocolLaunched])

    return (
        <div className={styles.launcherWrapper}>
            <div className={styles.loadingWrapper}>
                <Animation />
                <div className={styles.loadingText}>{loadingMessage}</div>
                {taskState === FractalAppState.FAILURE ||
                    (status === FractalTaskStatus.FAILURE && (
                        <div style={{ marginTop: 35 }}>
                            <button
                                type="button"
                                className={styles.greenButton}
                                onClick={tryAgain}
                            >
                                Try Again
                            </button>
                        </div>
                    ))}
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
}) => {
    return {
        userID: state.AuthReducer.user.userID,
        running: state.ContainerReducer.task.running,
        taskID: state.ContainerReducer.task.taskID,
        status: state.ContainerReducer.task.status,
        container: state.ContainerReducer.container,
    }
}

export default connect(mapStateToProps)(Launcher)
