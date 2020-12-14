/* eslint-disable import/no-named-as-default */

import React, { useState, useEffect, KeyboardEvent, Dispatch } from "react"
import { Row, Alert } from "react-bootstrap"
import { FaWifi, FaNetworkWired } from "react-icons/fa"
import { connect } from "react-redux"
import Slider from "react-input-slider"
import ToggleButton from "react-toggle-button"

import { config } from "shared/constants/config"
import { FractalClientCache } from "shared/types/cache"
import FractalKey from "shared/types/input"
import { openExternal } from "shared/utils/general/helpers"
import { ExternalAppType } from "store/reducers/types"
import { disconnectApp } from "store/actions/sideEffects"

import styles from "pages/dashboard/views/settings/settings.css"
import dashboardStyles from "pages/dashboard/dashboard.css"
import AdminSettings from "pages/dashboard/components/admin/adminSettings"

const Settings = (props: {
    dispatch: Dispatch<any>
    username: string
    accessToken: string
    externalApps: ExternalAppType[]
    connectedApps: string[]
}) => {
    const {
        dispatch,
        username,
        accessToken,
        externalApps,
        connectedApps,
    } = props

    const [lowInternetMode, setLowInternetMode] = useState(false)
    const [bandwidth, setBandwidth] = useState(500)
    const [showSavedAlert, setShowSavedAlert] = useState(false)

    const adminUsername =
        username &&
        username.indexOf("@") > -1 &&
        username.split("@")[1] === "tryfractal.com"

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
        <div
            className={dashboardStyles.page}
            style={{
                // might be nicer to add this to page but not sure if you prefer not to
                overflowY: "scroll",
                maxHeight: 525,
            }}
        >
            <h2 className={styles.title}>Streaming</h2>
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
            <Row className={styles.row}>
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
            {externalApps.length > -1 && (
                <>
                    <h2 className={styles.title} style={{ marginTop: 30 }}>
                        Cloud Storage
                    </h2>
                    <div className={styles.subtitle}>
                        Access, manage, edit, and share files from any of the
                        following cloud storage services within your streamed
                        apps by connecting your cloud drives to your Fractal
                        account.
                    </div>
                    {externalApps.map((externalApp: ExternalAppType) => (
                        <Row className={styles.row}>
                            <div style={{ width: "75%" }}>
                                <div className={styles.header}>
                                    <img
                                        alt="External app"
                                        src={externalApp.image_s3_uri}
                                        className={styles.cloudStorageIcon}
                                    />
                                    {externalApp.display_name}
                                </div>
                                <div className={styles.text}>
                                    By using this service through Fractal, you
                                    are agreeing to their{" "}
                                    <button
                                        type="button"
                                        className={styles.tosLink}
                                        onClick={() =>
                                            openExternal(externalApp.tos_uri)
                                        }
                                    >
                                        terms of service.
                                    </button>
                                </div>
                            </div>
                            <div style={{ width: "25%", alignSelf: "center" }}>
                                <div
                                    style={{
                                        float: "right",
                                    }}
                                >
                                    {connectedApps.indexOf(
                                        externalApp.code_name
                                    ) > -1 ? (
                                        <button
                                            type="button"
                                            className={styles.connectButton}
                                            onClick={() =>
                                                dispatch(
                                                    disconnectApp(
                                                        externalApp.code_name
                                                    )
                                                )
                                            }
                                        >
                                            Disconnect
                                        </button>
                                    ) : (
                                        <button
                                            type="button"
                                            className={styles.connectButton}
                                            onClick={() =>
                                                openExternal(
                                                    `${config.url.CLOUD_STORAGE_URL}external_app=${externalApp.code_name}&access_token=${accessToken}`
                                                )
                                            }
                                        >
                                            Connect
                                        </button>
                                    )}
                                </div>
                            </div>
                        </Row>
                    ))}
                </>
            )}
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
            <Row className={styles.row} style={{ justifyContent: "flex-end" }}>
                <div
                    role="button"
                    tabIndex={0}
                    onKeyDown={(event: KeyboardEvent<HTMLDivElement>) => {
                        if (event.key === FractalKey.ENTER) {
                            handleSave()
                        }
                    }}
                    className={dashboardStyles.feedbackButton}
                    style={{ width: 125, marginTop: 25 }}
                    onClick={handleSave}
                >
                    Save
                </div>
            </Row>
            {adminUsername && <AdminSettings />}
        </div>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        auth: { username: string; accessToken: string }
        apps: { external: ExternalAppType[]; connected: string[] }
    }
}) => {
    return {
        username: state.MainReducer.auth.username,
        accessToken: state.MainReducer.auth.accessToken,
        externalApps: state.MainReducer.apps.external,
        connectedApps: state.MainReducer.apps.connected,
    }
}

export default connect(mapStateToProps)(Settings)
