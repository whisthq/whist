import React, { useState, useEffect } from "react"
import { Row, Alert } from "react-bootstrap"
import { connect } from "react-redux"
import ToggleButton from "react-toggle-button"
import Slider from "react-input-slider"
import styles from "styles/dashboard.css"

import { updateAdmin } from "store/actions/pure"

import Wifi from "assets/images/wifi.svg"
import Speedometer from "assets/images/speedometer.svg"
import Fractal from "assets/images/fractal.svg"

const AdminSettings = (props: any) => {
    const { dispatch } = props

    const [region, setRegion] = useState("")
    const [task, setTask] = useState("")
    const [webserver, setWebserver] = useState("")
    const [cluster, setCluster] = useState("")

    const [showSavedAlert, setShowSavedAlert] = useState(false)

    const Input = (value: string, placeholder: string, onChange: any) => (
        <input
            value={value}
            placeholder={placeholder}
            onChange={onChange}
            style={{
                marginTop: 20,
                //width: "100%",
                background: "#F2F6FB",
                border: "none",
                outline: "none",
                fontSize: 14,
                color: "#black",
                //height: "160px",
            }}
        />
    )

    const updateRegion = (evt: any) => {
        setRegion(evt.target.value)
    }

    const updateTask = (evt: any) => {
        setTask(evt.target.value)
    }

    const updateWebserver = (evt: any) => {
        setWebserver(evt.target.value)
    }

    const updateCluster = (evt: any) => {
        setCluster(evt.target.value)
    }

    // admin settings that only admins will use
    // this should not be allowed to be called by randos
    const handleSave = () => {
        // update region if it's valid
        if (
            ["us-east-1", "us-west-1", "ca-central-1"].indexOf(
                region
            ) > -1 &&
            region.trim() !== ""
        ) {
            dispatch(
                updateAdmin({
                    region: region,
                })
            )
        } else if (region.toLowerCase() == "reset") {
            dispatch(
                updateAdmin({
                    region: null,
                })
            )
        }

        // update webserver url if it's valid
        let webserver_name: null | string = webserver.toLowerCase()

        // should be dev, prod, staging, or local
        if (webserver_name == "reset") {
            webserver_name = null
        }

        if (!webserver || webserver.trim() !== "") {
            dispatch(
                updateAdmin({
                    webserver_url: webserver,
                })
            )
        }

        if (cluster.toLowerCase() == "reset") {
            dispatch(
                updateAdmin({
                    cluster: null,
                })
            )
        } else if (cluster.trim() !== "") {
            dispatch(
                updateAdmin({
                    cluster: cluster,
                })
            )
        }

        //update task arn if it's valid
        //we want to copy the exact arn
        if (task.toLowerCase() == "reset") {
            dispatch(
                updateAdmin({
                    task_arn: null,
                })
            )
        } else if (task.trim() !== "") {
            dispatch(
                updateAdmin({
                    task_arn: task,
                })
            )
        }

        setShowSavedAlert(true)
    }


    return (
        <div>
            <Row
                style={{
                    display: "flex",
                    flexDirection: "column",
                    marginTop: 50,
                    marginBottom: 25,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 16,
                            fontWeight: "bold",
                        }}
                    >
                        <img
                            src={Fractal}
                            style={{
                                color: "#111111",
                                height: 14,
                                marginRight: 12,
                                position: "relative",
                                top: 2,
                                width: 16,
                            }}
                        />
                        (Admin Only) Test Settings
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: "#333333",
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        Give a region (like us-east-1) and a webserver
                        version (local, dev, staging, prod, or a url) and
                        save them to choose where to connect to. In future
                        may support different task defs. If the region is
                        not a valid region the change does not go through
                        (silently).
                    </div>
                </div>
                <div
                    style={{
                        width: "25%",
                        display: "flex",
                        flexDirection: "column",
                        justifyContent: "space-around",
                        alignItems: "flex-end",
                        padding: "10px 0px",
                    }}
                >
                    <div style={{ float: "right" }}>
                        {Input(
                            region,
                            "AWS Region",
                            updateRegion
                        )}
                        {Input(
                            webserver,
                            "Webserver",
                            updateWebserver
                        )}
                        {Input(
                            cluster,
                            "Cluster (optional)",
                            updateCluster
                        )}
                        {Input(
                            task,
                            "Task ARN",
                            updateTask
                        )}
                    </div>
                </div>
            </Row>
            {showSavedAlert && (
                // kind of just a copy of the way settings does it
                <Row>
                    <Alert
                        variant="success"
                        onClose={() => setShowSavedAlert(false)}
                        dismissible
                        style={{
                            width: "100%",
                            fontSize: 14,
                            borderRadius: 0,
                            border: "none",
                            padding: 20,
                            marginBottom: 0,
                        }}
                    >
                        Your settings have been saved!
                    </Alert>
                </Row>
            )}
            <Row style={{ justifyContent: "flex-end" }}>
                <div
                    className={styles.feedbackButton}
                    style={{ width: 110, marginTop: 25 }}
                    onClick={handleSave}
                >
                    SAVE
                </div>
            </Row>
        </div>    
    )
}

const Settings = (props: { username: string; dispatch: any }) => {
    const { username, dispatch } = props

    const [lowInternetMode, setLowInternetMode] = useState(false)
    const [bandwidth, setBandwidth] = useState(500)
    const [showSavedAlert, setShowSavedAlert] = useState(false)

    // these are used to allow admins to do integration testing
    // with the client app with
    // various different remotes
    const adminUsername =
        username &&
        username.indexOf("@") > -1 &&
        username.split("@")[1] == "tryfractal.com"    

    useEffect(() => {
        const Store = require("electron-store")
        const storage = new Store()

        const lowInternetMode = storage.get("lowInternetMode")
        const bandwidth = storage.get("bandwidth")

        if (lowInternetMode) {
            setLowInternetMode(lowInternetMode)
        }
        if (bandwidth) {
            setBandwidth(bandwidth)
        }
    }, [])

    const toggleLowInternetMode = (mode: boolean) => {
        setLowInternetMode(!mode)
    }

    const changeBandwidth = (mbps: number) => {
        setBandwidth(mbps)
    }

    const handleSave = () => {
        const Store = require("electron-store")
        const storage = new Store()
        const mbps = bandwidth === 50 ? 500 : bandwidth

        storage.set("lowInternetMode", lowInternetMode)
        storage.set("bandwidth", mbps)

        setShowSavedAlert(true)
    }

    return (
        <div
            className={styles.page}
            style={{
                overflowY: "scroll",
                maxHeight: 525,
            }}>
            <Row
                style={{
                    display: "flex",
                    flexDirection: "row",
                    marginTop: 50,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 16,
                            fontWeight: "bold",
                        }}
                    >
                        <img
                            src={Wifi}
                            style={{
                                color: "#111111",
                                height: 14,
                                marginRight: 12,
                                position: "relative",
                                top: 2,
                                width: 16,
                            }}
                        />
                        Low Internet Mode
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: "#333333",
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        If you have low Internet speeds (less than 15 Mbps) and
                        are experiencing stuttering, low Internet mode will
                        reduce Fractal's Internet speed requirement. Note that
                        this will increase your latency by a very small amount,
                        so we don't recommend activating it unless necessary.
                    </div>
                </div>
                <div style={{ width: "25%" }}>
                    <div style={{ float: "right", marginTop: "25px" }}>
                        <ToggleButton
                            value={lowInternetMode}
                            onToggle={toggleLowInternetMode}
                            colors={{
                                active: {
                                    base: "#93f1d9",
                                },
                                inactive: {
                                    base: "#161936",
                                },
                            }}
                            style={{ height: "40px" }}
                        />
                    </div>
                </div>
            </Row>
            <Row
                style={{
                    display: "flex",
                    flexDirection: "row",
                    marginTop: 50,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 16,
                            fontWeight: "bold",
                        }}
                    >
                        <img
                            src={Speedometer}
                            style={{
                                color: "#111111",
                                height: 14,
                                marginRight: 12,
                                position: "relative",
                                top: 2,
                                width: 16,
                            }}
                        />
                        Maximum Bandwidth
                    </div>
                    <div
                        style={{
                            fontSize: 13,
                            color: "#333333",
                            marginTop: 10,
                            marginLeft: 28,
                            lineHeight: 1.4,
                        }}
                    >
                        Toggle the maximum bandwidth (Mbps) that Fractal
                        consumes. We recommend adjusting this setting only if
                        you are also running other, bandwidth-consuming apps.
                    </div>
                </div>
                <div
                    style={{
                        width: "25%",
                        display: "flex",
                        flexDirection: "column",
                        justifyContent: "space-around",
                        alignItems: "flex-end",
                        padding: "10px 0px",
                    }}
                >
                    <div style={{ float: "right" }}>
                        <Slider
                            axis="x"
                            xstep={5}
                            xmin={10}
                            xmax={50}
                            x={bandwidth}
                            onChange={({ x }) => changeBandwidth(x)}
                            styles={{
                                track: {
                                    backgroundColor: "#cccccc",
                                    height: 5,
                                    width: 100,
                                },
                                active: {
                                    backgroundColor: "#93f1d9",
                                    height: 5,
                                },
                                thumb: {
                                    height: 10,
                                    width: 10,
                                },
                            }}
                        />
                    </div>
                    <div
                        style={{
                            fontSize: 11,
                            color: "#333333",
                            float: "right",
                            textAlign: "right",
                        }}
                    >
                        {bandwidth < 50 ? (
                            <div>{bandwidth} mbps</div>
                        ) : (
                            <div>Unlimited</div>
                        )}
                    </div>
                </div>
            </Row>
            {showSavedAlert && (
                <Row>
                    <Alert
                        variant="success"
                        onClose={() => setShowSavedAlert(false)}
                        dismissible
                        style={{
                            width: "100%",
                            fontSize: 14,
                            borderRadius: 0,
                            border: "none",
                            padding: 20,
                            marginBottom: 0,
                        }}
                    >
                        Your settings have been saved!
                    </Alert>
                </Row>
            )}
            <Row style={{ justifyContent: "flex-end" }}>
                <div
                    className={styles.feedbackButton}
                    style={{ width: 110, marginTop: 25 }}
                    onClick={handleSave}
                >
                    SAVE
                </div>
            </Row>
            {adminUsername && (
                <AdminSettings dispatch={dispatch} />
            )}
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
    }
}

export default connect(mapStateToProps)(Settings)
