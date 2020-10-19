import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Modal } from "react-bootstrap"
import styles from "styles/dashboard.css"

import Expand from "assets/images/expand.svg"

import { fetchContainer } from "store/actions/sideEffects"

const App = (props: any) => {
    const { dispatch, app } = props

    const [showModal, setShowModal] = useState(false)

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const handleLinkClick = () => {
        const { shell } = require("electron")
        shell.openExternal(app.url)
    }

    const handleLaunch = () => {
        dispatch(fetchContainer(app.app_id))
    }

    return (
        <div className={styles.appContainer}>
            <div
                style={{
                    position: "relative",
                    width: "70px",
                }}
                onClick={handleOpenModal}
            >
                <img src={app.logo_url} className={styles.appImage} />
            </div>
            <div className={styles.appText} onClick={handleOpenModal}>
                <div className={styles.appName}>{app.app_id}</div>
                <div className={styles.appDescription}>{app.description}</div>
                <div style={{ display: "flex", flexDirection: "row" }}>
                    <div className={styles.launchButton} onClick={handleLaunch}>
                        LAUNCH
                    </div>
                    <div
                        className={styles.expandButton}
                        onClick={handleOpenModal}
                    >
                        <img src={Expand} style={{ height: "15px" }} />
                    </div>
                </div>
            </div>
            <Modal show={showModal} onHide={handleCloseModal} size="lg">
                <Modal.Header
                    closeButton
                    style={{ border: "none", padding: "20px 40px 0 0" }}
                />
                <Modal.Body
                    style={{
                        display: "flex",
                        flexDirection: "column",
                        padding: "0px 40px 40px 40px",
                    }}
                >
                    <div style={{ display: "flex", flexDirection: "row" }}>
                        <div
                            style={{ minWidth: "120px", paddingRight: "20px" }}
                        >
                            <img
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
                                <span
                                    className={styles.appLink}
                                    onClick={handleLinkClick}
                                >
                                    {app.url}
                                </span>
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
                    <button
                        className={styles.modalButton}
                        onClick={handleLaunch}
                    >
                        LAUNCH
                    </button>
                </Modal.Body>
            </Modal>
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {}
}

export default connect(mapStateToProps)(App)
