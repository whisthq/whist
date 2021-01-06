import React, { useEffect, useState, Dispatch } from "react"
import { connect } from "react-redux"
import { useSpring, animated } from "react-spring"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import TitleBar from "shared/components/titleBar"
import { debugLog } from "shared/utils/general/logging"
import { updateContainer } from "store/actions/pure"
import { history } from "store/history"
import { execPromise } from "shared/utils/files/exec"
import { FractalRoute } from "shared/types/navigation"
import { OperatingSystem, FractalDirectory } from "shared/types/client"

import { useSubscription } from "@apollo/client"
import { SUBSCRIBE_USER_APP_STATE } from "shared/constants/graphql"

import styles from "pages/loading/loading.css"
import { cancelContainer, getStatus } from "store/actions/sideEffects"
import { FractalAppStates } from "shared/types/containers"
import { generateMessage } from "shared/components/loading"

const LOADING_TIMEOUT = 1000 * 200 // 200 seconds

const Loading = (props: {
    port32262: number
    port32263: number
    port32273: number
    ip: string
    secretKey: string
    desiredAppID: string
    currentAppID: string
    containerID: string
    statusID: string
    username: string
    pngFile: string
    dispatch: Dispatch<any>
}) => {
    const {
        port32262,
        port32263,
        port32273,
        ip,
        secretKey,
        desiredAppID,
        currentAppID,
        containerID,
        statusID,
        username,
        pngFile,
        dispatch,
    } = props

    // important note!!!
    // we are relying on containers automatically shutting down if no one connects
    // for the cancel button

    const [launches, setLaunches] = useState(0)
    const [status, setStatus] = useState(generateMessage())
    const [percentLoaded, setPercentLoaded] = useState(0)
    const [canLoad, setCanLoad] = useState(true)
    const [timedOut, setTimedOut] = useState(false)

    const percentLoadedWidth = 5 * percentLoaded

    const loadingBar = useSpring({ width: percentLoadedWidth })

    const { data, loading } = useSubscription(SUBSCRIBE_USER_APP_STATE, {
        variables: { userID: username },
    })
    /* Looks like:
    { "hardware_user_app_state":
      [
        {
        "state":"PENDING",
        "task_id":"5bc34a50-9452-4123-9ca8-6dd7af8c9495",
        "__typename":"hardware_user_app_state"
        }
      ]
    }
    */
    const state = data ? data.hardware_user_app_state[0].state : null
    const gqlTaskId = data ? data.hardware_user_app_state[0].task_id : null

    const rightTask = data && gqlTaskId && statusID && gqlTaskId === statusID
    const hasState = data && state

    const pending =
        loading ||
        !rightTask ||
        (hasState && state === FractalAppStates.PENDING)
    const ready = hasState && state === FractalAppStates.READY
    const cancelled = hasState && state === FractalAppStates.CANCELLED
    const failure = (hasState && state === FractalAppStates.FAILURE) || timedOut

    useEffect(() => {
        setTimeout(() => {
            if (canLoad && !timedOut) {
                setTimedOut(true)
            }
        }, LOADING_TIMEOUT)
    }, [])

    useEffect(() => {
        if (canLoad) {
            if (pending && percentLoaded < 100) {
                setTimeout(() => {
                    if (canLoad) {
                        setPercentLoaded(percentLoaded + 1)
                    }
                }, 1000) // every second 1 percent, change later
            } else if (ready) {
                setCanLoad(false)
                dispatch(getStatus(statusID))
                setPercentLoaded(100)
                setStatus("Stream successfully started.")
                setTimeout(() => null, 1000) // wait one sec so they can read the message
                setLaunches(launches + 1)
            } else if (cancelled) {
                setCanLoad(false)
                setStatus("Your app launch has been cancelled.")
                setPercentLoaded(0)
            } else if (failure) {
                setCanLoad(false)
                setStatus(
                    timedOut
                        ? "Unexpectedly failed to spin up your app."
                        : "Timed Out."
                )
                setPercentLoaded(0)
            } else {
                setStatus(
                    "Unexpecedly lost connection to server... trying to reconnect."
                )
            }
        }
    }, [percentLoaded, data, loading, timedOut])

    useEffect(() => {
        if (pending) {
            setTimeout(() => {
                if (canLoad) {
                    setStatus(generateMessage())
                }
            }, 5000)
        }
    }, [status])

    const resetLaunchRedux = () => {
        dispatch(
            updateContainer({
                containerID: null,
                cluster: null,
                port32262: null,
                port32263: null,
                port32273: null,
                publicIP: null,
                secretKey: null,
                launches: 0,
                launchURL: null,
                statusID: null,
            })
        )
    }

    const getExecutableName = (): string => {
        const currentOS = require("os").platform()

        if (currentOS === OperatingSystem.MAC) {
            return "./FractalClient"
        }
        if (currentOS === OperatingSystem.WINDOWS) {
            return "FractalClient.exe"
        }
        debugLog(`no suitable os found, instead got ${currentOS}`)
        return ""
    }

    const launchProtocol = () => {
        const spawn = require("child_process").spawn

        const protocolPath = require("path").join(
            FractalDirectory.getRootDirectory(),
            "protocol-build/desktop"
        )
        const executable = getExecutableName()

        execPromise("chmod +x FractalClient", protocolPath, [
            OperatingSystem.MAC,
            OperatingSystem.LINUX,
        ])
            .then(() => {
                const ipc = require("electron").ipcRenderer
                ipc.sendSync("canClose", false)

                const portInfo = `32262:${port32262}.32263:${port32263}.32273:${port32273}`
                const protocolParameters = {
                    w: 800,
                    h: 600,
                    p: portInfo,
                    k: secretKey,
                    n: `Fractalized ${desiredAppID}`,
                    ...(pngFile && { i: pngFile }),
                }

                const protocolArguments = [
                    ...Object.entries(protocolParameters)
                        .map(([flag, arg]) => [`-${flag}`, arg])
                        .flat(),
                    ip,
                ]

                // Starts the protocol
                const protocol = spawn(executable, protocolArguments, {
                    cwd: protocolPath,
                    detached: false,
                    stdio: "ignore",
                    // env: { ELECTRON_RUN_AS_NODE: 1 },
                    // optional:
                    // env: {
                    //    PATH: process.env.PATH,
                    // },
                })
                protocol.on("close", () => {
                    resetLaunchRedux()
                    setLaunches(0)
                    ipc.sendSync("canClose", true)
                    history.push(FractalRoute.DASHBOARD)
                })
                return null
            })
            .catch((error) => {
                throw error
            })
    }

    const returnToDashboard = () => {
        // emulates what the protocol would have done had you successfully closed
        // not sure if secretKey is the correct one, or if deleting while spinning up will work
        if (!failure) {
            dispatch(cancelContainer())
        }
        resetLaunchRedux()
        setLaunches(0)
        history.push(FractalRoute.DASHBOARD)
    }

    // TODO (adriano) graceful exit vs non graceful exit code
    // this should be done AFTER the endpoint to connect to EXISTS

    useEffect(() => {
        // Ensures that a container exists, that the protocol has not been launched before, and that
        // the app we want to launch is the app that will be launched
        if (containerID && launches === 0 && currentAppID === desiredAppID) {
            setLaunches(launches + 1)
        }
    }, [containerID])

    useEffect(() => {
        if (launches === 1) {
            launchProtocol()
        }
    }, [launches])

    return (
        <div
            style={{
                position: "fixed",
                top: 0,
                left: 0,
                width: 1000,
                height: 680,
                zIndex: 1000,
            }}
        >
            <TitleBar />
            <div className={styles.landingHeader}>
                <div className={styles.landingHeaderLeft}>
                    <span className={styles.logoTitle}>Fractal</span>
                </div>
            </div>
            <div style={{ position: "relative" }}>
                <div
                    style={{
                        paddingTop: 250,
                        textAlign: "center",
                        color: "white",
                        width: 1000,
                    }}
                >
                    <div
                        style={{
                            width: "500px",
                            margin: "auto",
                            background: "#cccccc",
                            height: "6px",
                        }}
                    >
                        <animated.div
                            style={loadingBar}
                            className={styles.loadingBar}
                        />
                    </div>
                    <div
                        style={{
                            marginTop: 10,
                            fontSize: 14,
                            display: "flex",
                            alignItems: "center",
                            justifyContent: "center",
                        }}
                    >
                        <div style={{ display: "flex", color: "#333333" }}>
                            {percentLoaded !== 100 && (
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{
                                        color: "#333333",
                                        marginRight: 10,
                                        width: 12,
                                        position: "relative",
                                        top: 3,
                                    }}
                                />
                            )}{" "}
                            {status}
                        </div>
                    </div>
                    <div style={{ display: "flex", justifyContent: "center" }}>
                        <div
                            role="button" // so eslint will not yell
                            tabIndex={0} // also for eslint
                            className={styles.dashboardButton}
                            style={{ width: 220, marginTop: 25 }}
                            onClick={returnToDashboard}
                            onKeyDown={returnToDashboard} // eslint
                        >
                            {failure ? "BACK TO DASHBOARD" : "CANCEL"}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

const mapStateToProps = (state: { MainReducer: Record<string, any> }) => {
    return {
        containerID: state.MainReducer.container.containerID,
        statusID: state.MainReducer.container.statusID,
        cluster: state.MainReducer.container.cluster,
        port32262: state.MainReducer.container.port32262,
        port32263: state.MainReducer.container.port32263,
        port32273: state.MainReducer.container.port32273,
        location: state.MainReducer.container.location,
        ip: state.MainReducer.container.publicIP,
        secretKey: state.MainReducer.container.secretKey,
        desiredAppID: state.MainReducer.container.desiredAppID,
        currentAppID: state.MainReducer.container.currentAppID,
        username: state.MainReducer.auth.username,
        accessToken: state.MainReducer.auth.accessToken,
        pngFile: state.MainReducer.container.pngFile,
    }
}

export default connect(mapStateToProps)(Loading)
