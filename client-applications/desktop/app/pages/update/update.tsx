import React, { useState, useEffect } from "react"

import { FractalIPC } from "shared/types/ipc"

import styles from "pages/update/update.css"

const Update = () => {
    /*
        Update screen that appears if an auto-update is available.
 
        Arguments: none
    */

    const [listenerCreated, setListenerCreated] = useState(false)
    const [percentageLeft, setPercentageLeft] = useState(500)
    const [percentageDownloaded, setPercentageDownloaded] = useState(10)
    const [downloadSpeed, setDownloadSpeed] = useState(0)
    const [transferred, setTransferred] = useState(0)
    const [total, setTotal] = useState(0)
    const [downloadError, setDownloadError] = useState("")

    const ipc = require("electron").ipcRenderer

    useEffect(() => {
        ipc.sendSync(FractalIPC.SHOW_MAIN_WINDOW)
    }, [])

    useEffect(() => {
        if (!listenerCreated) {
            setListenerCreated(true)
            ipc.on(
                FractalIPC.PERCENT_DOWNLOADED,
                (_: IpcRendererEvent, percent: number) => {
                    percent *= 3
                    setPercentageLeft(500 - percent)
                    setPercentageDownloaded(percent)
                }
            )

            ipc.on(
                FractalIPC.DOWNLOAD_SPEED,
                (_: IpcRendererEvent, speed: number) => {
                    setDownloadSpeed((speed / 1000000).toFixed(2))
                }
            )

            ipc.on(
                FractalIPC.PERCENT_TRANSFERRED,
                (_: IpcRendererEvent, reportedTransferred: number) => {
                    setTransferred((reportedTransferred / 1000000).toFixed(2))
                }
            )

            ipc.on(
                FractalIPC.TOTAL_DOWNLOADED,
                (_: IpcRendererEvent, reportedTotal: number) => {
                    setTotal((reportedTotal / 1000000).toFixed(2))
                }
            )

            ipc.on(
                FractalIPC.DOWNLOAD_ERROR,
                (_: IpcRendererEvent, error: string) => {
                    setDownloadError(error)
                }
            )
        }
    }, [listenerCreated])

    return (
        <div className={styles.updateWrapper}>
            <div className={styles.updateCentered}>
                <div className={styles.loadingBar}>
                    <div
                        style={{
                            width: `${percentageDownloaded.toString()}px`,
                        }}
                        className={styles.remaining}
                    />
                    <div
                        style={{
                            width: `${percentageLeft.toString()}px`,
                        }}
                        className={styles.downloaded}
                    />
                </div>
                {downloadError === "" ? (
                    <div className={styles.updateTextWrapper}>
                        <div style={{ color: "white" }}>
                            Downloading Update ({downloadSpeed.toString()} Mbps)
                        </div>
                        <div className={styles.updateSubtext}>
                            {transferred.toString()} /{total.toString()} MB
                            Downloaded
                        </div>
                    </div>
                ) : (
                    <div className={styles.updateTextWrapper}>
                        <div style={{ color: "white" }}>{downloadError}</div>
                    </div>
                )}
            </div>
        </div>
    )
}

export default Update
