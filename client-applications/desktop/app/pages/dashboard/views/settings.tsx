import React, { useState, useEffect } from "react"
import { Row, Alert } from "react-bootstrap"
import { connect } from "react-redux"
import ToggleButton from "react-toggle-button"
import Slider from "react-input-slider"
import styles from "styles/dashboard.css"

import Wifi from "assets/images/wifi.svg"
import Speedometer from "assets/images/speedometer.svg"
import Fractal from "assets/images/fractal.svg"

const Settings = (props: { username: string; dispatch: any }) => {
    const { username, dispatch } = props

    const adminUsername =
        username &&
        username.indexOf("@") > -1 &&
        username.split("@")[1] == "tryfractal.com"

    const [lowInternetMode, setLowInternetMode] = useState(false)
    const [bandwidth, setBandwidth] = useState(500)
    const [showSavedAlert, setShowSavedAlert] = useState(false)

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

    const handleSave = () => {
        const storage = require("electron-json-storage")
        const mbps = bandwidth === 50 ? 500 : bandwidth
        storage.set("settings", {
            lowInternetMode: lowInternetMode,
            bandwidth: mbps,
        })
        setShowSavedAlert(true)
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
                        flexDirection: "row",
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
                            Give a region, webserver and task to run. When you
                            launch, the app will connect to said webserver and
                            open said task in said region. More options will be
                            added in the future.
                        </div>
                        {/* <input />
                        <input />
                        <input /> */}
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
