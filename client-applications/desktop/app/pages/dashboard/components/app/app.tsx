import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Col, Modal, Tooltip, OverlayTrigger } from "react-bootstrap"
import { FaPlay, FaCheck } from "react-icons/fa"

import { createContainer } from "store/actions/sideEffects"
import { updateContainer } from "store/actions/pure"
import { history } from "store/history"
import { FractalRoute } from "shared/types/navigation"
import { openExternal } from "shared/utils/helpers"
import { FractalApp } from "shared/types/ui"
import { OperatingSystem } from "shared/types/client"
import { createShortcutName } from "shared/utils/shortcuts"

import styles from "pages/dashboard/components/app/app.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

/* eslint-disable react/jsx-props-no-spreading */

const App = (props: {
    app: FractalApp
    launches: number
    clientOS: OperatingSystem
    dispatch: Dispatch
    admin: boolean
}) => {
    const { app, launches, clientOS, admin, dispatch } = props

    const [showModal, setShowModal] = useState(false)
    const [launched, setLaunched] = useState(false)

    const tooltip =
        clientOS === OperatingSystem.WINDOWS
            ? `Look for "${createShortcutName(
                  app.app_id
              )}" on your desktop or Start Menu.`
            : `Look for "${createShortcutName(
                  app.app_id
              )}" on your desktop or Applications folder.`

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const handleLinkClick = (url: string) => {
        openExternal(url)
    }

    const handleLaunch = () => {
        setLaunched(true)
        dispatch(updateContainer({ launches: launches + 1 }))
    }

    useEffect(() => {
        if (launches === 1 && launched) {
            history.push(FractalRoute.LOADING)
            dispatch(createContainer(app.app_id, null, admin))
            setLaunched(false)
        }
    }, [launches, launched])

    return (
        <Col xs={3}>
            <div>
                <button
                    type="button"
                    className={styles.playButton}
                    onClick={handleLaunch}
                >
                    <div style={{ position: "relative" }}>
                        <div
                            style={{
                                position: "absolute",
                                top: "50%",
                                left: "50%",
                                transform: "translate(-50%, -55%)",
                            }}
                        >
                            <FaPlay className={styles.faPlayButton} />
                        </div>
                    </div>
                </button>
                <div
                    style={{ height: 220, paddingBottom: 20, marginBottom: 10 }}
                >
                    <button
                        type="button"
                        className={styles.appContainer}
                        onClick={handleOpenModal}
                    >
                        <div className={styles.appHeading}>
                            <img
                                src={app.logo_url}
                                className={styles.appImage}
                                alt="logo"
                            />
                            <div className={styles.appName}>{app.app_id}</div>
                            <div className={styles.appDescription}>
                                {app.description}
                            </div>
                        </div>
                    </button>
                </div>
            </div>
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
        </Col>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        container: {
            launches: number
        }
        client: {
            clientOS: OperatingSystem
        }
    }
}) => {
    return {
        launches: state.MainReducer.container.launches,
        clientOS: state.MainReducer.client.clientOS,
    }
}

export default connect(mapStateToProps)(App)
