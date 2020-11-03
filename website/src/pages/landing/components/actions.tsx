import React, { useState, useContext, useEffect } from "react"
import { connect } from "react-redux"
import { Button, Modal, Alert } from "react-bootstrap"
import { CopyToClipboard } from "react-copy-to-clipboard"

import * as PureAuthAction from "store/actions/auth/pure"
import * as SideEffectWaitlistAction from "store/actions/waitlist/sideEffects"

import WaitlistForm from "shared/components/waitlistForm"
import history from "shared/utils/history"

import { REFERRAL_POINTS, SIGNUP_POINTS } from "shared/utils/points"
import MainContext from "shared/context/mainContext"
import { config } from "shared/constants/config"

import ReactGA from "react-ga"

import { categories, actions, ga_event } from "shared/constants/gaEvents"

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
                        fontSize: width > 720 ? 17 : 16,
                        textAlign: "left",
                    }}
                >
                    {text}
                </div>
                <div className="points" style={{ textAlign: "right" }}>
                    +{points.toString()} pts
                </div>
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
    waitlistUser: {
        user_id: string
        referralCode: string
        points: number
        referrals: number
        name: string
        authEmail: string
    }
}) => {
    const { dispatch, waitlistUser } = props

    const [showModal, setShowModal] = useState(false)
    const [showEmailSentAlert, setShowEmailSentAlert] = useState(false)
    const [recipientEmail, setRecipientEmail] = useState("")
    const [sentEmail, setSentEmail] = useState("")

    const handleOpenModal = () => setShowModal(true)
    const handleCloseModal = () => setShowModal(false)

    useEffect(() => {
        if (!waitlistUser.authEmail) {
            dispatch(PureAuthAction.resetUser())
        }
    }, [dispatch, waitlistUser])

    const gaLogSentReferallEmail = () => {
        ReactGA.event(
            ga_event(categories.POINTS, actions.POINTS.SENT_REFERALL_EMAIL)
        )
    }

    const updateRecipientEmail = (evt: any) => {
        evt.persist()
        setRecipientEmail(evt.target.value)
    }

    const sendReferralEmail = () => {
        if (waitlistUser.user_id && recipientEmail) {
            dispatch(SideEffectWaitlistAction.sendReferralEmail(recipientEmail))
            setSentEmail(recipientEmail)
            setShowEmailSentAlert(true)

            gaLogSentReferallEmail()
        }
    }

    const renderActions = () => {
        if (waitlistUser && waitlistUser.user_id) {
            return (
                <div style={{ width: "100%" }}>
                    {!waitlistUser.authEmail && (
                        <CustomAction
                            onClick={() => history.push("/auth")}
                            text="Create an account"
                            points={SIGNUP_POINTS}
                        />
                    )}
                    <CustomAction
                        onClick={handleOpenModal}
                        text="Refer a Friend"
                        points={REFERRAL_POINTS}
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
    WaitlistReducer: {
        waitlistUser: any
    }
}) {
    return {
        waitlistUser: state.WaitlistReducer.waitlistUser,
    }
}

export default connect(mapStateToProps)(Actions)
