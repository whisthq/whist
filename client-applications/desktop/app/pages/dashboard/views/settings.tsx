import React, { useState, useEffect } from "react"
import { Row, Alert } from "react-bootstrap"
import { connect } from "react-redux"
import ToggleButton from "react-toggle-button"
import Slider from "react-input-slider"
import styles from "styles/dashboard.css"

import { webserver_urls } from "shared/constants/config"

import { updateAdmin, updateClient } from "store/actions/pure"

import Wifi from "assets/images/wifi.svg"
import Speedometer from "assets/images/speedometer.svg"
import Fractal from "assets/images/fractal.svg"

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
    
    const [admin_region, admin_setRegion] = useState("")
    const [admin_task, admin_setTask] = useState("")
    const [admin_webserver, admin_setWebserver] = useState("")

    useEffect(() => {
        const storage = require("electron-json-storage")
        storage.get("settings", (error: any, data: any) => {
            if (error) throw error
            if (data && data.lowInternetMode && data.bandwidth) {
                setLowInternetMode(data.lowInternetMode)
                setBandwidth(data.bandwidth)
            }
        })
    }, [])

    const toggleLowInternetMode = (mode: boolean) => {
        setLowInternetMode(!mode)
    }

    const changeBandwidth = (mbps: number) => {
        setBandwidth(mbps)
    }

    const admin_updateRegion = (evt: any) => {
        admin_setRegion(evt.target.value)
    }

    const admin_updateTask = (evt: any) => {
        admin_setTask(evt.target.value)
    }

    const admin_updateWebserver = (evt: any) => {
        admin_setWebserver(evt.target.value)
    }

    const adminInput = (value: string, placeholder: string, onChange: any) => (
        <input value={value}
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

    const handleSave = () => {
        const storage = require("electron-json-storage")
        const mbps = bandwidth === 50 ? 500 : bandwidth
        storage.set("settings", {
            lowInternetMode: lowInternetMode,
            bandwidth: mbps,
        })
        setShowSavedAlert(true)

        // below here is a bunch of admin stuff that allows us to integration test better
        if (adminUsername) {
            // update region if it's valid
            if (["us-east-1", "us-west-1", "ca-central-1"].indexOf(admin_region) > -1) {
                dispatch(updateAdmin({
                    region: admin_region,
                }))
            }

            // update webserver url if it's valid
            let webserver = admin_webserver.toLowerCase()

            if (webserver == "dev" || webserver == "develop") {
                webserver = webserver_urls.dev
            } else if (webserver == "staging") {
                webserver = webserver_urls.staging
            } else if (webserver == "prod" || webserver == "production") {
                webserver = webserver_urls.production
            } else if (webserver == "local") {
                webserver = webserver = webserver_urls.local
            }
            
            // if we didn't change it and had this length it was probably a url
            if (webserver.length > 10 ){
                dispatch(updateAdmin({
                    webserver_url: webserver,
                }))
            }

            //update task arn if it's valid
            if (admin_task.length > 10){
                dispatch(updateAdmin({
                    task_arn : admin_task,
                }))
            }
        }
    }

    return (
        <div className={styles.page}>
            <h2>App Settings </h2>
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
            {adminUsername && (
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
                            Give a region (like us-east-1) and a webserver version 
                            (local, dev, staging, prod, or a url) and save them to choose where
                            to connect to. In future may support different task defs. If the region
                            is not a valid region the change does not go through (silently).
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
                            {adminInput(admin_region, "AWS Region", admin_updateRegion)}
                            {adminInput(admin_webserver, "Webserver", admin_updateWebserver)}
                            {adminInput(admin_task, "Task ARN", admin_updateTask)}
                        </div>
                    </div>
                </Row>
            )}
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
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {
        username: state.MainReducer.auth.username,
    }
}

export default connect(mapStateToProps)(Settings)
