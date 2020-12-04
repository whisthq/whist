import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import Titlebar from "react-electron-titlebar"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

import { OperatingSystem } from "shared/types/client"
import styles from "pages/login/login.css"

const Update = (props: { clientOS: string; needsUpdate: boolean }) => {
    const { clientOS, needsUpdate } = props

    const [percentageLeft, setPercentageLeft] = useState(500)
    const [percentageDownloaded, setPercentageDownloaded] = useState(0)
    const [downloadSpeed, setDownloadSpeed] = useState("0")
    const [transferred, setTransferred] = useState("0")
    const [total, setTotal] = useState("0")
    const [downloadError, setDownloadError] = useState("")

    useEffect(() => {
        const ipc = require("electron").ipcRenderer

        ipc.on("percent", (_: IpcRendererEvent, percent: number) => {
            percent *= 3
            setPercentageLeft(500 - percent)
            setPercentageDownloaded(percent)
        })

        ipc.on("download-speed", (_: IpcRendererEvent, speed: number) => {
            setDownloadSpeed((speed / 1000000).toFixed(2))
        })

        ipc.on(
            "transferred",
            (_: IpcRendererEvent, reportedTransferred: number) => {
                setTransferred((reportedTransferred / 1000000).toFixed(2))
            }
        )

        ipc.on("total", (_: IpcRendererEvent, reportedTotal: number) => {
            setTotal((reportedTotal / 1000000).toFixed(2))
        })

        ipc.on("error", (_: IpcRendererEvent, error: string) => {
            setDownloadError(error)
        })

        // ipc.on("downloaded", (event, downloaded) => {});
    }, [])

    return (
        <div>
            {needsUpdate ? (
                <div
                    style={{
                        position: "fixed",
                        top: 0,
                        left: 0,
                        width: 1000,
                        height: 680,
                        backgroundColor: "white",
                        zIndex: 1000,
                    }}
                >
                    {clientOS === OperatingSystem.WINDOWS ? (
                        <div>
                            <Titlebar backgroundColor="#000000" />
                        </div>
                    ) : (
                        <div style={{ marginTop: 10 }} />
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
                                    display: "flex",
                                    alignItems: "center",
                                    justifyContent: "center",
                                }}
                            >
                                <div
                                    style={{
                                        width: `${percentageDownloaded}px`,
                                        height: 6,
                                        background: "#EFEFEF",
                                    }}
                                />
                                <div
                                    style={{
                                        width: `${percentageLeft}px`,
                                        height: 6,
                                        background: "#111111",
                                    }}
                                />
                            </div>
                            {downloadError === "" ? (
                                <div
                                    style={{
                                        marginTop: 10,
                                        fontSize: 14,
                                        display: "flex",
                                        alignItems: "center",
                                        justifyContent: "center",
                                    }}
                                >
                                    <div style={{ color: "#333333" }}>
                                        <FontAwesomeIcon
                                            icon={faCircleNotch}
                                            spin
                                            style={{
                                                color: "#333333",
                                                marginRight: 4,
                                                width: 12,
                                            }}
                                        />{" "}
                                        Downloading Update ({downloadSpeed}{" "}
                                        Mbps)
                                    </div>
                                </div>
                            ) : (
                                <div
                                    style={{
                                        marginTop: 10,
                                        fontSize: 14,
                                        display: "flex",
                                        alignItems: "center",
                                        justifyContent: "center",
                                    }}
                                >
                                    <div style={{ color: "#333333" }}>
                                        {downloadError}
                                    </div>
                                </div>
                            )}
                            <div
                                style={{
                                    color: "#333333",
                                    fontSize: 10,
                                    margin: "auto",
                                    marginTop: 5,
                                }}
                            >
                                {transferred} /{total} MB Downloaded
                            </div>
                        </div>
                    </div>
                </div>
            ) : (
                <div />
            )}
        </div>
    )
}

export const mapStateToProps = <T extends {}>(state: T) => {
    return {
        clientOS: state.MainReducer.clientOS,
    }
}

export default connect(mapStateToProps)(Update)
