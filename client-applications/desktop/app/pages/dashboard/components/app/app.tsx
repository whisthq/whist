import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import { Col, Tooltip, OverlayTrigger } from "react-bootstrap"
import { FaPlay } from "react-icons/fa"

import { createContainer } from "store/actions/sideEffects"
import { updateContainer } from "store/actions/pure"
import { history } from "store/history"
import { FractalRoute } from "shared/types/navigation"
import { FractalApp, FractalAppLocalState } from "shared/types/ui"
import { SVGConverter } from "shared/utils/files/images"
import AppPopup from "pages/dashboard/components/app/components/appPopup"

import styles from "pages/dashboard/components/app/app.css"

const App = (props: {
    app: FractalApp
    launches: number
    admin: boolean
    dispatch: Dispatch<any>
}) => {
    const { app, launches, admin, dispatch } = props

    const [showModal, setShowModal] = useState(false)
    const [launched, setLaunched] = useState(false)

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const handleLaunch = () => {
        setLaunched(true)
        dispatch(updateContainer({ launches: launches + 1 }))
    }

    useEffect(() => {
        if (launches === 1 && launched) {
            history.push(FractalRoute.LOADING)
            // Create PNG
            SVGConverter.saveAsPngTemp(app.logo_url, app.app_id)
                .then((pngPath: string) => {
                    // Create container
                    dispatch(updateContainer({ pngFile: pngPath }))
                    dispatch(createContainer(app.app_id, null, admin))
                    setLaunched(false)
                    return null
                })
                .catch((err) => {
                    throw err
                })
        }
    }, [launches, launched])

    return (
        <Col xs={3}>
            <div>
                {app.localState === FractalAppLocalState.INSTALLED && (
                    <div className={styles.installed}>Installed</div>
                )}
                <OverlayTrigger
                    placement="top"
                    overlay={
                        <Tooltip id="button-tooltip">
                            <div className={styles.tooltipText}>
                                Click to preview {app.app_id} without
                                downloading.
                            </div>
                        </Tooltip>
                    }
                >
                    <button
                        type="button"
                        className={styles.playButton}
                        onClick={handleLaunch}
                    >
                        <div style={{ position: "relative" }}>
                            <div className={styles.playButtonWrapper}>
                                <FaPlay className={styles.faPlayButton} />
                            </div>
                        </div>
                    </button>
                </OverlayTrigger>

                <div className={styles.appContainerWrapper}>
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
            <AppPopup
                app={app}
                showModal={showModal}
                callback={handleCloseModal}
            />
        </Col>
    )
}

const mapStateToProps = (state: {
    MainReducer: {
        container: {
            launches: number
        }
    }
}) => {
    return {
        launches: state.MainReducer.container.launches,
    }
}

export default connect(mapStateToProps)(App)
