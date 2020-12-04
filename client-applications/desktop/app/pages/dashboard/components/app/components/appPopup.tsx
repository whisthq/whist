import React from "react"
import { connect } from "react-redux"
import { Modal, Tooltip, OverlayTrigger } from "react-bootstrap"
import { FaCheck } from "react-icons/fa"

import { openExternal } from "shared/utils/helpers"
import { FractalApp } from "shared/types/ui"
import { OperatingSystem } from "shared/types/client"
import { createShortcutName } from "shared/utils/shortcuts"

import styles from "pages/dashboard/components/app/components/appPopup.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

const AppPopup = (props: {
    app: FractalApp
    showModal: boolean
    clientOS: OperatingSystem
    callback: (showModal: boolean) => void
}) => {
    const { app, showModal, clientOS, callback } = props

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
                    {app.installed ? (
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
        }
    }
}) => {
    return {
        clientOS: state.MainReducer.client.clientOS,
    }
}

export default connect(mapStateToProps)(AppPopup)
