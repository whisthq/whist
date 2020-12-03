import React, { useState, useEffect, KeyboardEvent } from "react"
import { Row, Alert } from "react-bootstrap"
import { connect } from "react-redux"
import ToggleButton from "react-toggle-button"
import Slider from "react-input-slider"
import { FaWifi, FaNetworkWired } from "react-icons/fa"

import { FractalClientCache } from "shared/types/cache"
import FractalKey from "shared/types/input"

import styles from "pages/dashboard/views/settings/settings.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

const Settings = () => {
    const [lowInternetMode, setLowInternetMode] = useState(false)
    const [bandwidth, setBandwidth] = useState(500)
    const [showSavedAlert, setShowSavedAlert] = useState(false)

    useEffect(() => {
        const Store = require("electron-store")
        const storage = new Store()

        const cachedLowInternetMode = storage.get(
            FractalClientCache.LOW_INTERNET_MODE
        )
        const cachedBandwidth = storage.get(FractalClientCache.BANDWIDTH)

        if (cachedLowInternetMode) {
            setLowInternetMode(cachedLowInternetMode)
        }
        if (cachedBandwidth) {
            setBandwidth(cachedBandwidth)
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

        storage.set(FractalClientCache.LOW_INTERNET_MODE, lowInternetMode)
        storage.set(FractalClientCache.BANDWIDTH, mbps)

        setShowSavedAlert(true)
    }

    return (
        <div className={dashboardStyles.page}>
            <Row className={styles.row}>
                <div style={{ width: "75%" }}>
                    <div className={styles.header}>
                        <FaWifi className={styles.faIcon} />
                        Low Internet Mode
                    </div>
                    <div className={styles.text}>
                        If you have low Internet speeds (less than 15 Mbps), low
                        Internet mode will reduce Fractal&lsquo;s Internet speed
                        requirement. This will increase your latency slightly,
                        so we don&lsquo;t recommend activating it unless
                        necessary.
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
                className={styles.row}
                style={{
                    marginBottom: 25,
                }}
            >
                <div style={{ width: "75%" }}>
                    <div className={styles.header}>
                        <FaNetworkWired className={styles.faIcon} />
                        Maximum Bandwidth
                    </div>
                    <div className={styles.text}>
                        Toggle the maximum bandwidth (Mbps) that Fractal
                        consumes. We recommend adjusting this setting only if
                        you are also running other, bandwidth-consuming apps.
                    </div>
                </div>
                <div className={styles.sliderWrapper}>
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
                    <div className={styles.sliderPopup}>
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
                        className={styles.alert}
                    >
                        Your settings have been saved!
                    </Alert>
                </Row>
            )}
            <Row style={{ justifyContent: "flex-end" }}>
                <div
                    role="button"
                    tabIndex={0}
                    onKeyDown={(event: KeyboardEvent<HTMLDivElement>) => {
                        if (event.key === FractalKey.ENTER) {
                            handleSave()
                        }
                    }}
                    className={dashboardStyles.feedbackButton}
                    style={{ width: 110, marginTop: 25 }}
                    onClick={handleSave}
                >
                    Save
                </div>
            </Row>
        </div>
    )
}

const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(Settings)
