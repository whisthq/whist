import React, { useState } from "react"
import { connect } from "react-redux"
import { Form } from "react-bootstrap"
import DivSpace from "shared/components/divSpace"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { Redirect } from "react-router"

import { GrSend } from "react-icons/gr"

import Header from "shared/components/header"

import { submitFeedback } from "store/actions/customer/sideEffects"

import "styles/shared.css"
import { checkEmail } from "pages/auth/constants/authHelpers"

// TODO please tell them they have to be logged in
const Support = (props: {
    user: any
    dispatch: any
}) => {
    const { dispatch, user } = props

    const [email, setEmail] = useState("")
    const [request, setRequest] = useState("")
    const [processing, setProcessing] = useState(false)
    const [sentRequest, setSentRequest] = useState(false)

    // send to support@fractalcomputers.com
    // TODO this is going to have to be changed
    // because we want many people to be able to submit questions
    // we'll want one feedback or support email and one questions email
    const sendRequest = () => {
        if (request.length >= 7 && checkEmail(email)) {
            setProcessing(true)
            dispatch(submitFeedback(request))
            setTimeout(() => {
                setSentRequest(true)
                setTimeout(() => {
                    // reset the entire state
                    setSentRequest(false)
                    setProcessing(false)
                    setEmail("")
                    setRequest("")
                }, 2000)
            }, 3000)
        }
    }

    // Handles ENTER key press
    const onKeyPress = (evt: any) => {
        if (evt.key === "Enter") {
            sendRequest()
        }
    }

    // Updates email and password fields as user types
    const changeEmail = (evt: any): any => {
        evt.persist()
        setEmail(evt.target.value)
    }

    const changeRequest = (evt: any): any => {
        evt.persist()
        setRequest(evt.target.value)
    }

    if (!user.user_id && user.accessToken) {
        // tell them somehow that this is a requirement
        return <Redirect to="/" />
    } else if (sentRequest) {
        // YEA TODO PLEASE MAKE A VIEW (or another component below)
        // ALSO TODO ADD COOL ICONS
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <div
                    style={{
                        width: 400,
                        margin: "auto",
                        marginTop: 70,
                    }}
                >
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 32,
                            fontWeight: "bold",
                            textAlign: "center",
                        }}
                    >
                        Sent!
                    </div>
                </div>
            </div>
        )
    } else if (processing) {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <PuffAnimation />
            </div>
        )
    } else {
        // TODO (adriano) consider putting the form in its own component for brevity
        return (
            // I think the value stuff works not sure though
            <div className="fractalContainer">
                <Header color="black" />
                <div
                    style={{
                        width: 400,
                        margin: "auto",
                        marginTop: 70,
                    }}
                >
                    <div
                        style={{
                            color: "#111111",
                            fontSize: 32,
                            fontWeight: "bold",
                            textAlign: "center",
                        }}
                    >
                        Support Form
                    </div>
                    <DivSpace height={40} />
                    <Form>
                        <Form.Group controlId="Please Enter Your Email Address">
                            <Form.Label>Please Enter Email Address</Form.Label>
                            <Form.Control
                                type="email"
                                placeholder="bob@tryfractal.com"
                                value={email}
                                onChange={changeEmail}
                                onKeyPress={onKeyPress}
                            />
                        </Form.Group>
                        <Form.Group controlId="Please Enter Your Request">
                            <Form.Label>Please Enter Your Request</Form.Label>
                            <Form.Control
                                as="textarea"
                                rows={10}
                                value={request}
                                onChange={changeRequest}
                                onKeyPress={onKeyPress}
                            />
                        </Form.Group>
                    </Form>
                    <button
                        style={{
                            width: "100%",
                            opacity:
                                checkEmail(email) && request.length >= 7
                                    ? 1
                                    : 0.5,
                        }}
                        onClick={() => {
                            sendRequest()
                        }}
                        disabled={!checkEmail(email) || request.length < 7}
                        className="google-button" // reuse this for now
                    >
                        <GrSend
                            style={{
                                color: "white",
                                position: "relative",
                                top: 5,
                                marginRight: 15,
                                // TODO LOOKS WTFWTF
                            }}
                        />
                        <div>Send Request</div>
                    </button>
                </div>
            </div>
        )
    }
}


function mapStateToProps(state: { AuthReducer: { authFlow: any; user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Support)
