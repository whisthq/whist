import React, { useState, useEffect, useCallback } from "react"
import { connect } from "react-redux"
import { useSpring, animated } from "react-spring"
import styles from "styles/login.css"
import Titlebar from "react-electron-titlebar"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"
import { deleteContainer } from "store/actions/sideEffects"

const UpdateScreen = (props: any) => {
    const {
        os,
        username,
        percentLoaded,
        status,
        container_id,
        port32262,
        port32263,
        port32273,
        width, // for the screen
        height, // for the screen
        ip,
    } = props

    // figure out how to use useEffect
    // note to future developers: setting state inside useffect when you rely on
    // change for those variables to trigger runs forever and is bad
    // use two variables for that or instead do something like this below
    var percentLoadedWidth = 5 * percentLoaded

    const loadingBar = useSpring({ width: percentLoadedWidth })

    useEffect(() => {
        if (container_id) {
            LaunchProtocol()
        }
    }, [container_id])

    const LaunchProtocol = () => {
        var child = require("child_process").spawn
        var appRootDir = require("electron").remote.app.getAppPath()
        var executable = ""
        var path = ""

        const os = require("os")

        if (os.platform() === "darwin") {
            console.log("darwin found")
            path = appRootDir + "/protocol-build/desktop/"
            path = path.replace("/app", "")
            executable = "./FractalClient"
        } else if (os.platform() === "linux") {
            console.log("linux found")
            path = process.cwd() + "/protocol-build"
            path = path.replace("/release", "")
            executable = "./FractalClient"
        } else if (os.platform() === "win32") {
            console.log("windows found")
            path = process.cwd() + "\\protocol-build\\desktop"
            executable = "FractalClient.exe"
        } else {
            console.log(`no suitable os found, instead got ${os.platform()}`)
        }

        var port_info = `32262:${port32262},32263:${port32263},32273:${port32273}`
        console.log(port_info)
        var parameters = ["-w", 800, "-h", 600, "-p", port_info, ip]
        console.log(parameters)
        console.log(`your executable path should be: ${path}`)

        // Starts the protocol
        const protocol1 = child(executable, parameters, {
            cwd: path,
            detached: true,
            stdio: "ignore",
            // optional:
            //env: {
            //    PATH: process.env.PATH,
            //},
        })
        protocol1.on("close", (code: any) => {
            console.log("the protocol has been closed!")
            deleteContainer(username, container_id)
        })
        console.log("spawn completed!")

        // TODO (adriano) graceful exit vs non graceful exit code
        // this should be done AFTER the endpoint to connect to EXISTS
    }

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
            {os === "win32" ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div style={{ marginTop: 10 }}></div>
            )}
            <div className={styles.landingHeader}>
                <div className={styles.landingHeaderLeft}>
                    <span className={styles.logoTitle}>Fractal</span>
                </div>
            </div>
            <div style={{ position: "relative" }}>
                <div
                    style={{
                        paddingTop: 180,
                        textAlign: "center",
                        color: "white",
                        width: 1000,
                    }}
                >
                    <div
                        style={{
                            padding: "2px",
                            borderRadius: "5px",
                            width: "500px",
                            margin: "auto",
                            border: "solid 0.8px #555555",
                        }}
                    >
                        <animated.div
                            style={loadingBar}
                            className={styles.loadingBar}
                        ></animated.div>
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
                        <div style={{ color: "#555555" }}>
                            {status != "Successfully created container." &&
                                status != "Successfully deleted container." && (
                                    <FontAwesomeIcon
                                        icon={faCircleNotch}
                                        spin
                                        style={{
                                            color: "#5EC4EB",
                                            marginRight: 4,
                                            width: 12,
                                        }}
                                    />
                                )}{" "}
                            {status}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

function mapStateToProps(state: any) {
    console.log(state)
    return {
        os: state.MainReducer.client.os,
        username: state.MainReducer.auth.username,
        percentLoaded: state.MainReducer.loading.percentLoaded,
        status: state.MainReducer.loading.statusMessage,
        container_id: state.MainReducer.container.container_id,
        cluster: state.MainReducer.container.cluster,
        port32262: state.MainReducer.container.port32262,
        port32263: state.MainReducer.container.port32263,
        port32273: state.MainReducer.container.port32273,
        location: state.MainReducer.container.location,
        ip: state.MainReducer.container.publicIP,
    }
}

export default connect(mapStateToProps)(UpdateScreen)
