import React, { useState, useContext } from "react"
import { connect } from "react-redux"
import { Button, Modal } from "react-bootstrap"
import { CopyToClipboard } from "react-copy-to-clipboard"

import GoogleButton from "pages/auth/googleButton"

import MainContext from "shared/context/mainContext"
import { config } from "constants/config"

const ReferAction = (props: { onClick: any }) => {
    const { width } = useContext(MainContext)

    return (
        <Button className="action" onClick={props.onClick}>
            <div
                style={{
                    fontSize: width > 720 ? 22 : 16,
                }}
            >
                Refer a Friend
            </div>
            <div style={{ color: "#3930b8", fontWeight: "bold" }}>
                +100 points
            </div>
        </Button>
    )
}

const JoinWaitlistAction = () => {
    const { width } = useContext(MainContext)

    const scrollToTop = () => {
        window.scrollTo({
            top: 0,
            behavior: "smooth",
        })
    }

    return (
        <Button onClick={scrollToTop} className="action">
            <div
                style={{
                    fontSize: width > 720 ? 22 : 16,
                }}
            >
                Join Waitlist
            </div>
            <div style={{ color: "#3930b8", fontWeight: "bold" }}>
                {" "}
                +100 points
            </div>
        </Button>
    )
}

const Actions = (props: {
    user: { email: string; referralCode: string }
    loggedIn: any
}) => {
    const [showModal, setShowModal] = useState(false)

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const renderActions = () => {
        if (props.user && props.user.email) {
            if (props.loggedIn) {
                return <ReferAction onClick={handleOpenModal} />
            } else {
                return (
                    <>
                        <GoogleButton />
                        <ReferAction onClick={handleOpenModal} />
                    </>
                )
            }
        } else {
            return <JoinWaitlistAction />
        }
    }

    return (
        <div
            style={{
                display: "flex",
                flexDirection: "column",
                alignItems: "flex-start",
                marginTop: 50,
            }}
        >
            {renderActions()}

            <Modal show={showModal} onHide={handleCloseModal}>
                <Modal.Header closeButton>
                    <Modal.Title>Refer a Friend</Modal.Title>
                </Modal.Header>
                <Modal.Body>
                    <div>
                        Want to move up the waitlist? Refer a friend by sending
                        them your referral code. Once they join and enter your
                        referral code, you'll receive 100 points!
                    </div>
                    <br />
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "row",
                            alignItems: "center",
                        }}
                    >
                        <div className="code-container">
                            {config.url.FRONTEND_URL +
                                "/" +
                                props.user.referralCode}
                        </div>
                        <CopyToClipboard
                            text={
                                config.url.FRONTEND_URL +
                                "/" +
                                props.user.referralCode
                            }
                        >
                            <Button className="modal-button">Copy Code</Button>
                        </CopyToClipboard>
                    </div>
                </Modal.Body>
                <Modal.Footer>
                    <Button className="modal-button" onClick={handleCloseModal}>
                        Got it
                    </Button>
                </Modal.Footer>
            </Modal>
        </div>
    )
}

function mapStateToProps(state: {
    AuthReducer: { user: any; logged_in: any }
}) {
    return {
        user: state.AuthReducer.user,
        loggedIn: state.AuthReducer.logged_in,
    }
}

export default connect(mapStateToProps)(Actions)
