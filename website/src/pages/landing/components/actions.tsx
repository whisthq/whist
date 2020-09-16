import React, { useState } from "react"
import { connect } from "react-redux"
import { Button, Modal } from "react-bootstrap"
import { CopyToClipboard } from "react-copy-to-clipboard"

import GoogleButton from "pages/auth/googleButton"

const ReferAction = (props: { onClick: any }) => {
    return (
        <Button className="action" onClick={props.onClick}>
            <div
                style={{
                    color: "white",
                    fontSize: "23px",
                }}
            >
                Refer a Friend
            </div>
            <div style={{ color: "#00D4FF" }}> +100 points</div>
        </Button>
    )
}

const JoinWaitlistAction = () => {
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
                    color: "white",
                    fontSize: "23px",
                }}
            >
                Join Waitlist
            </div>
            <div style={{ color: "#00D4FF" }}> +50 points</div>
        </Button>
    )
}

const Actions = (props: { user: { email: any }; loggedIn: any }) => {
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
            }}
        >
            <h1
                style={{
                    fontWeight: "bold",
                    color: "#111111",
                    marginBottom: "30px",
                    fontSize: "40px",
                }}
            >
                Actions
            </h1>
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
                        <div className="code-container">KG7GSJ2</div>
                        <CopyToClipboard text="KG7GSJ2">
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
