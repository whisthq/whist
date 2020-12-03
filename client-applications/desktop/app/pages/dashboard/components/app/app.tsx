import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import { Col, Modal } from "react-bootstrap"
import { FaPlay } from "react-icons/fa"

import { createContainer } from "store/actions/sideEffects"
import { updateContainer } from "store/actions/pure"
import { history } from "store/history"
import { FractalRoute } from "shared/types/navigation"
import { openExternal } from "shared/utils/helpers"
import { FractalApp } from "shared/types/ui"

import styles from "pages/dashboard/components/app/app.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

const App = (props: {
    app: FractalApp
    launches: number
    dispatch: Dispatch
}) => {
    const { app, launches, dispatch } = props

    const [showModal, setShowModal] = useState(false)
    const [launched, setLaunched] = useState(false)

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
            dispatch(createContainer(app.app_id))
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
                                transform: "translate(-50%, -50%)",
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
                            agreeing to their{" "}
                        </div>
                        <button
                            type="button"
                            className={styles.tosLink}
                            onClick={() => handleLinkClick(app.tos)}
                        >
                            terms of service.
                        </button>
                    </div>
                    <button
                        type="button"
                        className={dashboardStyles.modalButton}
                    >
                        Download
                    </button>
                </Modal.Body>
            </Modal>
        </Col>
    )
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        launches: state.MainReducer.container.launches,
    }
}

export default connect(mapStateToProps)(App)
