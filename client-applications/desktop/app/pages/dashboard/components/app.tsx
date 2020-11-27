import React, { useState, useEffect } from "react"
import { Col } from "react-bootstrap"
import { connect } from "react-redux"
import { Modal } from "react-bootstrap"
import styles from "styles/dashboard.css"

import { createContainer } from "store/actions/sideEffects"
import { updateContainer } from "store/actions/pure"
import { history } from "store/configureStore"

const App = (props: any) => {
    const { dispatch, app, launches } = props

    const [showModal, setShowModal] = useState(false)
    const [launched, setLaunched] = useState(false)

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const handleLinkClick = (url: string) => {
        const { shell } = require("electron")
        shell.openExternal(url)
    }

    const handleLaunch = () => {
        setLaunched(true)
        dispatch(updateContainer({ launches: launches + 1 }))
    }

    useEffect(() => {
        if (launches === 1 && launched) {
            history.push("/loading")
            dispatch(createContainer(app.app_id))
            setLaunched(false)
        }
    }, [launches, launched])

    return (
        <Col xs={4}>
            <div className={styles.appContainer} onClick={handleOpenModal}>
                <div className={styles.appHeading}>
                    <img src={app.logo_url} className={styles.appImage} />
                    <div className={styles.appName}>{app.app_id}</div>
                </div>
                <div className={styles.appDescription}>{app.description}</div>
                <button className={styles.launchButton} onClick={handleLaunch}>
                    LAUNCH
                </button>
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
                                    onClick={() => handleLinkClick(app.url)}
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
                    <div className={styles.tos}>
                        Note: By using this app through Fractal, you are
                        agreeing to their{" "}
                        <span
                            className={styles.tosLink}
                            onClick={() => handleLinkClick(app.tos)}
                        >
                            terms of service.
                        </span>
                    </div>
                    <button
                        className={styles.modalButton}
                        onClick={handleLaunch}
                    >
                        LAUNCH
                    </button>
                </Modal.Body>
            </Modal>
        </Col>
    )
}

const mapStateToProps = (state: any) => {
    return {
        launches: state.MainReducer.container.launches,
    }
}

export default connect(mapStateToProps)(App)
