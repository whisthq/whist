import React, { useEffect, useState, Dispatch } from "react"
import { connect } from "react-redux"
import { useSpring, animated } from "react-spring"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import TitleBar from "shared/components/titleBar"
import { debugLog } from "shared/utils/logging"
import { updateContainer, updateLoading } from "store/actions/pure"
import { history } from "store/history"
import { execChmodUnix } from "shared/utils/exec"
import { FractalRoute } from "shared/types/navigation"
import { OperatingSystem } from "shared/types/client"

import { useSubscription } from "@apollo/client"
import { SUBSCRIBE_USER_APP_STATE } from "shared/constants/graphql"

import styles from "pages/login/login.css"
import { cancelContainer, getStatus } from "store/actions/sideEffects"
import { FractalAppStates } from "shared/types/containers"
import { generateMessage } from "shared/utils/loading"

// TODO we need some way to signal the 500 thing (or maybe we do just let it keep loading? prolly not)
// const warning = json && json.state === FractalStatus.FAILURE ? "Unexpectedly failed to start stream. Close and try again."
//     : response && response.status && response.status === 500
//         ? `(${moment().format("hh:mm:ss")}) ` +
//             "Unexpectedly lost connection with server. Please close the app and try again."
//         : "Server unexpectedly not responding. Close the app and try again."

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
    dispatch: Dispatch
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
        dispatch,
    } = props

    // figure out how to use useEffect
    // note to future developers: setting state inside useffect when you rely on
    // change for those variables to trigger runs forever and is bad
    // use two variables for that or instead do something like this below

    const [launches, setLaunches] = useState(0)
    const [status, setStatus] = useState(generateMessage())
    const [percent, setPercent] = useState(0)

    const percentLoadedWidth = 5 * percent

    const loadingBar = useSpring({ width: percentLoadedWidth })

    const { data, loading } = useSubscription(SUBSCRIBE_USER_APP_STATE, {
        variables: { userID: username },
    })

    useEffect(() => {
        if (percent < 100) {
            setTimeout(() => setPercent(percent + 1), 1000) // every second 1 percent, change later
        }
    }, [percent])

    useEffect(() => {
        // TODO this is busted it needs to check for state being bad
        setTimeout(() => setStatus(generateMessage()), 5000) // every 5 sec change status
    }, [status])

    useEffect(() => {
        const rightTask =
            data && data.task_id && statusID && data.task_id === statusID
        const hasState = data && data.state
        if (
            loading ||
            (hasState && rightTask && data.state === FractalAppStates.PENDING)
        ) {
            dispatch(
                updateLoading({
                    percentLoaded: percent,
                    statusMessage: generateMessage(),
                })
            )
            // show a loading message
        } else if (hasState) {
            // we'll just put the loading stuff by default
            if (data.state === FractalAppStates.READY) {
                // just launched and is ready to go
                dispatch(getStatus(statusID))
                if (status === "Stream successfully started.") {
                    // change this
                    setLaunches(launches + 1)
                }
            } else if (data.state === FractalAppStates.CANCELLED) {
                dispatch(
                    updateLoading({
                        statusMessage: "Your Launch has been cancelled.",
                    })
                )
            } else if (data.state === FractalAppStates.FAILURE) {
                dispatch(
                    updateLoading({
                        statusMessage:
                            "Unexpectedly failed to spin up your app.",
                    })
                )
            }
        } else {
            // show a loading or a disconnected message
        }
    }, [data, loading, percent])

    const failedToLaunch = data && data.state === FractalAppStates.FAILURE

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
            })
        )
        dispatch(
            updateLoading({
                statusMessage: "Powering up your app",
                percentLoaded: 0,
            })
        )
    }

    const LaunchProtocol = () => {
        const child = require("child_process").spawn
        const appRootDir = require("electron").remote.app.getAppPath()
        let executable = ""
        let path = ""

        const os = require("os")

        if (os.platform() === OperatingSystem.MAC) {
            path = `${appRootDir}/protocol-build/desktop/`
            path = path.replace("/app", "")
            path = path.replace("/Resources.asar", "")
            executable = "./FractalClient"
        } else if (os.platform() === OperatingSystem.WINDOWS) {
            path = `${appRootDir}\\protocol-build\\desktop`
            path = path.replace("\\resources\\app.asar", "")
            path = path.replace("\\app\\protocol-build", "\\protocol-build")
            executable = "FractalClient.exe"
        } else {
            debugLog(`no suitable os found, instead got ${os.platform()}`)
        }

        execChmodUnix("chmod +x FractalClient", path, os.platform())
            .then(() => {
                const ipc = require("electron").ipcRenderer
                ipc.sendSync("canClose", false)

                const portInfo = `32262:${port32262}.32263:${port32263}.32273:${port32273}`
                const parameters = [
                    "-w",
                    800,
                    "-h",
                    600,
                    "-p",
                    portInfo,
                    "-k",
                    secretKey,
                    ip,
                ]

                // Starts the protocol
                const protocol = child(executable, parameters, {
                    cwd: path,
                    detached: false,
                    stdio: "ignore",
                    // env: { ELECTRON_RUN_AS_NODE: 1 },
                    // optional:
                    // env: {
                    //    PATH: process.env.PATH,
                    // },
                })
                return protocol.on("close", () => {
                    resetLaunchRedux()
                    setLaunches(0)
                    ipc.sendSync("canClose", true)
                    history.push(FractalRoute.DASHBOARD)
                })
            })
            .catch((error) => {
                throw error
            })
    }

    const returnToDashboard = () => {
        // emulates what the protocol would have done had you successfully closed
        // not sure if secretKey is the correct one, or if deleting while spinning up will work
        if (!failedToLaunch) {
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
            LaunchProtocol()
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
                            {failedToLaunch ? "BACK TO DASHBOARD" : "CANCEL"}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

const mapStateToProps = <T extends {}>(state: T) => {
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
        user: state.MainReducer.auth.username,
    }
}

export default connect(mapStateToProps)(Loading)
