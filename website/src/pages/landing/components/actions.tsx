import React, { useState, useContext } from "react"
import { connect } from "react-redux"
import { Button, Modal, Alert } from "react-bootstrap"
import { CopyToClipboard } from "react-copy-to-clipboard"
import { useMutation } from "@apollo/client"

import * as PureWaitlistAction from "store/actions/waitlist/pure"
import * as SideEffectWaitlistAction from "store/actions/waitlist/sideEffects"

import GoogleButton from "pages/auth/googleButton"
import WaitlistForm from "shared/components/waitlistForm"

import { UPDATE_WAITLIST } from "shared/constants/graphql"
import { REFERRAL_POINTS } from "shared/utils/points"
import MainContext from "shared/context/mainContext"
import { config } from "shared/constants/config"

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
    user: {
        user_id: string
        referralCode: string
        points: number
        referrals: number
        name: string
    }
    clicks: any
    waitlistUser: any
}) => {
    const { dispatch, user, clicks, waitlistUser } = props

    const [showModal, setShowModal] = useState(false)
    const [showEmailSentAlert, setShowEmailSentAlert] = useState(false)
    const [recipientEmail, setRecipientEmail] = useState("")
    const [sentEmail, setSentEmail] = useState("")
    const [warning, setWarning] = useState("")

    const [updatePoints] = useMutation(UPDATE_WAITLIST, {
        onError(err) {},
    })

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    const increasePoints = () => {
        if (user) {
            var allowClick = true
            var clickReset = false
            const currentTime = new Date().getTime() / 1000

            if (currentTime - clicks.lastClicked > 1) {
                if (clicks.number > 50) {
                    if (currentTime - clicks.lastClicked > 60 * 60 * 3) {
                        clickReset = true
                    } else {
                        allowClick = false
                        setWarning(
                            "Max clicks reached! Clicking will reset in 3 hours."
                        )
                    }
                }

                if (allowClick) {
                    if (clickReset) {
                        dispatch(PureWaitlistAction.updateClicks({ number: 1 }))
                    } else {
                        dispatch(
                            PureWaitlistAction.updateClicks({
                                number: clicks.number + 1,
                            })
                        )
                    }

                    updatePoints({
                        variables: {
                            user_id: user.user_id,
                            points: waitlistUser.points + 1,
                            referrals: waitlistUser.referrals,
                        },
                    })
                }
            }
        }
    }

    const updateRecipientEmail = (evt: any) => {
        evt.persist()
        setRecipientEmail(evt.target.value)
    }

    const sendReferralEmail = () => {
        if (user.user_id && recipientEmail) {
            dispatch(SideEffectWaitlistAction.sendReferralEmail(recipientEmail))
            setSentEmail(recipientEmail)
            setShowEmailSentAlert(true)
        }
    }

    const renderActions = () => {
        if (user && user.user_id) {
            return (
                <div style={{ width: "100%" }}>
                    {<GoogleButton />}
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

            <Modal size="lg" show={showModal} onHide={handleCloseModal}>
                <Modal.Header closeButton>
                    <Modal.Title>Refer a Friend</Modal.Title>
                </Modal.Header>
                <Modal.Body>
                    <div>
                        Want to move up the waitlist? Refer a friend by sending
                        them your referral code. Once they join and enter your
                        referral code, you'll receive {REFERRAL_POINTS} points!
                    </div>
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "row",
                            alignItems: "center",
                            marginTop: "25px",
                            marginBottom: "25px",
                        }}
                    >
                        <div className="code-container">
                            {config.url.FRONTEND_URL +
                                "/" +
                                waitlistUser.referralCode}
                        </div>
                        <CopyToClipboard
                            text={
                                config.url.FRONTEND_URL +
                                "/" +
                                waitlistUser.referralCode
                            }
                        >
                            <Button className="modal-button">Copy Link</Button>
                        </CopyToClipboard>
                    </div>
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "row",
                            alignItems: "center",
                        }}
                    >
                        <input
                            type="text"
                            placeholder="Friend's Email Address"
                            onChange={updateRecipientEmail}
                            className="code-container"
                        />
                        <Button
                            className="modal-button"
                            onClick={sendReferralEmail}
                        >
                            Send Invite Email
                        </Button>
                    </div>
                    {showEmailSentAlert && (
                        <Alert
                            variant="success"
                            onClose={() => setShowEmailSentAlert(false)}
                            dismissible
                        >
                            Your email to <b>{sentEmail}</b> has been sent!
                        </Alert>
                    )}
                </Modal.Body>
                <Modal.Footer>
                    <Button
                        className="modal-button"
                        onClick={handleCloseModal}
                        style={{ width: "100px" }}
                    >
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
    }
    WaitlistReducer: {
        waitlistUser: any
        clicks: any
    }
}) {
    return {
        user: state.AuthReducer.user,
        waitlistUser: state.WaitlistReducer.waitlistUser,
        clicks: state.WaitlistReducer.clicks,
    }
}

export default connect(mapStateToProps)(Actions)
