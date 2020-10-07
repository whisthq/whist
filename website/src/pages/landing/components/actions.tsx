import React, { useState, useContext } from "react"
import { connect } from "react-redux"
import { Button, Modal } from "react-bootstrap"
import { CopyToClipboard } from "react-copy-to-clipboard"

import GoogleButton from "pages/auth/googleButton"
import WaitlistForm from "shared/components/waitlistForm"

import { db } from "shared/utils/firebase"

import { REFERRAL_POINTS } from "shared/utils/points"
import MainContext from "shared/context/mainContext"
import { config } from "constants/config"

const CustomAction = (props: { onClick: any; text: any; points: number }) => {
    const { onClick, text, points } = props
    const { width } = useContext(MainContext)

    return (
        <button className="action" onClick={onClick}>
            <div
                style={{
                    fontSize: width > 720 ? 20 : 16,
                }}
            >
                {text}
            </div>
            <div className="points">+{points.toString()} points</div>
        </button>
    )
}

const Actions = (props: {
    user: { email: string; referralCode: string }
    loggedIn: any
    unsortedLeaderboard: any
}) => {
    const { user, loggedIn, unsortedLeaderboard } = props

    const [showModal, setShowModal] = useState(false)

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const increasePoints = () => {
        if (user) {
            const email = user.email

            const hasClicked = unsortedLeaderboard[email].hasOwnProperty(
                "clicks"
            )

            if (!hasClicked) {
                unsortedLeaderboard[email] = {
                    ...unsortedLeaderboard[email],
                    clicks: 1,
                    points: unsortedLeaderboard[email].points + 1,
                }
            } else {
                unsortedLeaderboard[email] = {
                    ...unsortedLeaderboard[email],
                    clicks: unsortedLeaderboard[email].clicks + 1,
                    points: unsortedLeaderboard[email].points + 1,
                }
            }

            db.collection("metadata").doc("waitlist").update({
                leaderboard: unsortedLeaderboard,
            })
        }
    }

    const renderActions = () => {
        if (user && user.email) {
            return (
                <div style={{ width: "100%" }}>
                    {!loggedIn && <GoogleButton />}
                    <CustomAction
                        onClick={handleOpenModal}
                        text="Refer a Friend"
                        points={REFERRAL_POINTS}
                    />
                    <CustomAction
                        onClick={increasePoints}
                        text={
                            <div>
                                <div>Click Me</div>
                            </div>
                        }
                        points={1}
                    />
                </div>
            )
        } else {
            return <WaitlistForm isAction />
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
                        referral code, you'll receive 300 points!
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
                            {config.url.FRONTEND_URL + "/" + user.referralCode}
                        </div>
                        <CopyToClipboard
                            text={
                                config.url.FRONTEND_URL +
                                "/" +
                                user.referralCode
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
    AuthReducer: { user: any; loggedIn: any; unsortedLeaderboard: any }
}) {
    return {
        user: state.AuthReducer.user,
        loggedIn: state.AuthReducer.loggedIn,
        unsortedLeaderboard: state.AuthReducer.unsortedLeaderboard,
    }
}

export default connect(mapStateToProps)(Actions)
