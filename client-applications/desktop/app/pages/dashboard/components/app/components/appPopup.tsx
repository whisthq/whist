import React, { useState } from "react"
import { connect } from "react-redux"
import { Modal, Tooltip, OverlayTrigger } from "react-bootstrap"
import { FaCheck } from "react-icons/fa"

import { openExternal, updateArrayByKey } from "shared/utils/helpers"
import { FractalApp, FractalAppLocalState } from "shared/types/ui"
import { OperatingSystem, FractalWindowsDirectory } from "shared/types/client"
import {
    createShortcutName,
    createShortcut,
    createDirectorySync,
} from "shared/utils/shortcuts"
import { updateClient } from "store/actions/pure"

import styles from "pages/dashboard/components/app/components/appPopup.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

const AppPopup = (props: {
    app: FractalApp
    apps: FractalApp[]
    showModal: boolean
    clientOS: OperatingSystem
    callback: (showModal: boolean) => void
    dispatch: Dispatch
}) => {
    const { app, apps, showModal, clientOS, callback, dispatch } = props

    const [shortcutCreated, setShortcutCreated] = useState(false)

    const [png, setPng] = useState("")

    const tooltip =
        clientOS === OperatingSystem.WINDOWS
            ? `Look for "${createShortcutName(
                  app.app_id
              )}" on your desktop or Start Menu.`
            : `Look for "${createShortcutName(
                  app.app_id
              )}" on your desktop or Applications folder.`

    const handleCloseModal = () => callback(false)

    const handleLinkClick = (url: string) => {
        openExternal(url)
    }

    const handleDownload = () => {
        if (clientOS === OperatingSystem.MAC) {
            debugLog("not yet implemented")
        } else if (clientOS === OperatingSystem.WINDOWS) {
            const outputPath = `${FractalWindowsDirectory.START_MENU}Fractal\\`

            setShortcutCreated(true)
            // Create a Fractal directory in the Start Menu if one doesn't exist
            if (
                !createDirectorySync(
                    FractalWindowsDirectory.START_MENU,
                    "Fractal"
                )
            ) {
                return
            }

            // Create the shortcut inside the Fractal Directory
            createShortcut(app, outputPath, (shortcutCreated) => {
                if (shortcutCreated) {
                    const { array, index } = updateArrayByKey(
                        apps,
                        "app_id",
                        app.app_id,
                        {
                            localState: FractalAppLocalState.INSTALLED,
                        }
                    )

                    if (array && index !== 0) {
                        dispatch(updateClient({ apps: array }))
                    }
                }
            })
        }
    }

    return (
        <>
            <Modal
                show={showModal}
                onHide={handleCloseModal}
                size="lg"
                style={{ marginTop: 100 }}
            >
                <Modal.Header
                    closeButton
                    style={{ border: "none", padding: "20px 40px 0 0" }}
                />
                <Modal.Body
                    style={{
                        display: "flex",
                        flexDirection: "column",
                        padding: "0px 40px 40px 40px",
                        border: "none",
                    }}
                >
                    <div style={{ display: "flex", flexDirection: "row" }}>
                        <div
                            style={{ minWidth: "120px", paddingRight: "20px" }}
                        >
                            <img
                                alt="logo"
                                src={app.logo_url}
                                className={styles.modalAppImage}
                            />
                        </div>
                        <div>
                            <h1>{app.app_id}</h1>
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "row",
                                    fontSize: "13px",
                                }}
                            >
                                <div style={{ color: "#cccccc" }}>
                                    {app.category}
                                </div>
                                <button
                                    type="button"
                                    className={styles.appLink}
                                    onClick={() => handleLinkClick(app.url)}
                                >
                                    {app.url}
                                </button>
                            </div>
                        </div>
                    </div>
                    <div
                        style={{
                            minHeight: "150px",
                            marginTop: "30px",
                            marginBottom: "50px",
                        }}
                    >
                        {app.long_description}
                    </div>
                    <div className={styles.tos}>
                        <div>
                            Note: By using this app through Fractal, you are
                            agreeing to their
                        </div>
                        <button
                            type="button"
                            className={styles.tosLink}
                            onClick={() => handleLinkClick(app.tos)}
                        >
                            terms of service.
                        </button>
                    </div>
                    {app.localState === FractalAppLocalState.INSTALLED ? (
                        <OverlayTrigger
                            placement="top"
                            overlay={
                                <Tooltip id="button-tooltip">
                                    <div className={styles.tooltipText}>
                                        {tooltip}
                                    </div>
                                </Tooltip>
                            }
                        >
                            <div className={dashboardStyles.installedButton}>
                                <div>Installed</div>
                                <div>
                                    <FaCheck className={styles.faCheck} />
                                </div>
                            </div>
                        </OverlayTrigger>
                    ) : (
                        <button
                            type="button"
                            className={dashboardStyles.modalButton}
                            onClick={handleDownload}
                            style={{
                                background:
                                    shortcutCreated && "rgba(0,0,0,0.05)",
                            }}
                        >
                            Download
                        </button>
                    )}
                </Modal.Body>
            </Modal>
        </>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        client: {
            clientOS: OperatingSystem
            apps: FractalApp[]
        }
    }
}) => {
    return {
        clientOS: state.MainReducer.client.clientOS,
        apps: state.MainReducer.client.apps,
    }
}

export default connect(mapStateToProps)(AppPopup)
