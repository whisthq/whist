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
import { updateClicks } from "store/actions/auth/waitlist"

const CustomAction = (props: {
    onClick: any
    text: any
    points: number
    warning?: string
}) => {
    const { onClick, text, points, warning } = props
    const { width } = useContext(MainContext)

    return (
        <button className="action" onClick={onClick}>
            <div
                style={{
                    display: "flex",
                    flexDirection: "row",
                    width: "100%",
                    justifyContent: "space-between",
                }}
            >
                <div
                    style={{
                        fontSize: width > 720 ? 20 : 16,
                    }}
                >
                    {text}
                </div>
                <div className="points">+{points.toString()} points</div>
            </div>
            {warning && warning !== "" && (
                <div
                    style={{
                        width: "100%",
                        fontWeight: "normal",
                        fontSize: 12,
                        textAlign: "left",
                    }}
                >
                    {warning}
                </div>
            )}
        </button>
    )
}

const Actions = (props: {
    dispatch: any
    user: { email: string; referralCode: string }
    loggedIn: any
    unsortedLeaderboard: any
    clicks: any
}) => {
    const { dispatch, user, loggedIn, unsortedLeaderboard, clicks } = props

    const [showModal, setShowModal] = useState(false)
    const [warning, setWarning] = useState("")

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const increasePoints = () => {
        if (user) {
            var allowClick = true
            const currentTime = new Date().getTime() / 1000

            if (clicks.number > 50) {
                if (currentTime - clicks.lastClicked > 60 * 60 * 3) {
                    dispatch(updateClicks(0))
                } else {
                    allowClick = false
                    setWarning(
                        "Max clicks reached! Clicking will reset in 3 hours."
                    )
                }
            }

            if (allowClick) {
                dispatch(updateClicks(clicks.number + 1))

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
                        warning={warning}
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
                        referral code, you'll receive {REFERRAL_POINTS} points!
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
    AuthReducer: {
        user: any
        loggedIn: any
        unsortedLeaderboard: any
        clicks: number
    }
}) {
    return {
        user: state.AuthReducer.user,
        loggedIn: state.AuthReducer.loggedIn,
        unsortedLeaderboard: state.AuthReducer.unsortedLeaderboard,
        clicks: state.AuthReducer.clicks
            ? state.AuthReducer.clicks
            : {
                  number: 0,
                  lastClicked: 0,
              },
    }
}

export default connect(mapStateToProps)(Actions)
