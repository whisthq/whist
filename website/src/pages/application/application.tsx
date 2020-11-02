import React, { useState, useContext } from "react"
import { Row, Col, Button, FormControl } from "react-bootstrap"
import { connect } from "react-redux"
//import { HashLink } from "react-router-hash-link"

import history from "shared/utils/history"
import { db } from "shared/utils/firebase"

import "styles/application.css"

import MainContext from "shared/context/mainContext"
import Header from "shared/components/header"

function Application(props: any) {
    const { waitlistUser } = props
    const { width } = useContext(MainContext)

    const [devices, setDevices] = useState("")
    const [apps, setApps] = useState("")
    const [source, setSource] = useState("")

    const [flowState, setFlowState] = useState(1)

    function changeDevices(evt: any) {
        evt.persist()
        setDevices(evt.target.value)
    }

    function changeApps(evt: any) {
        evt.persist()
        setApps(evt.target.value)
    }

    function changeSource(evt: any) {
        evt.persist()
        setSource(evt.target.value)
    }

    function submitForm() {
        db.collection("essays")
            .doc(waitlistUser.user_id)
            .set({
                devices: devices,
                apps: apps,
                source: source,
            })
            .then(() => history.push("/"))
    }

    const form =
        flowState === 1 ? (
            <div>
                <h2>Thanks for signing up with us, {waitlistUser.name}!</h2>
                <p style={{ marginTop: 35 }}>
                    As a starter, tell us why you're excited to try Fractal!
                    What are the top three apps you'd like us to stream to you?
                </p>
                <div className="application-form">
                    <FormControl
                        as="textarea"
                        onChange={changeApps}
                        value={apps}
                        rows={3}
                        placeholder="Any favorite apps? Common ones include Chrome, Figma, Blender, Steam, VSCode, and Slack."
                        style={{
                            resize: "none",
                            outline: "none",
                            background: "rgba(213, 217, 253, 0.2)",
                            border: "none",
                            marginTop: 35,
                            padding: "20px 30px",
                        }}
                    />
                </div>
            </div>
        ) : flowState === 2 ? (
            // consider adding emojis!
            <div>
                <h2>What devices would you use Fractal on?</h2>
                <p style={{ marginTop: 35 }}>
                    By tailoring Fractal to your device we can improve your
                    experience with system-specific features and better
                    networking.
                </p>
                <div className="application-form">
                    <FormControl
                        as="textarea"
                        onChange={changeDevices}
                        rows={3}
                        value={devices}
                        placeholder="Windows PCs, Macs, Chromebooks, tablets and more, even your phone."
                        style={{
                            resize: "none",
                            outline: "none",
                            background: "rgba(213, 217, 253, 0.2)",
                            border: "none",
                            marginTop: 35,
                            padding: "20px 30px",
                        }}
                    />
                </div>
            </div>
        ) : flowState === 3 ? (
            <div>
                <h2>How did you hear about us?</h2>
                <p style={{ marginTop: 35 }}>
                    Please be as specific as possible. For instance, if you
                    found us through a Google search, what keywords were you
                    searching for?
                </p>
                <div className="application-form">
                    <FormControl
                        as="textarea"
                        value={source}
                        onChange={changeSource}
                        rows={3}
                        placeholder="I heard about Fractal through..."
                        style={{
                            resize: "none",
                            outline: "none",
                            background: "rgba(213, 217, 253, 0.2)",
                            border: "none",
                            marginTop: 35,
                            padding: "20px 30px",
                        }}
                    />
                </div>
            </div>
        ) : (
            <div>
                <h2>
                    Congrats, {waitlistUser.name}! You're <br />
                    <span
                        style={{
                            color: "#3930b8",
                            fontSize: width > 720 ? "calc(140px + 2.2vw)" : 50,
                        }}
                    >
                        No. {waitlistUser.ranking}
                    </span>{" "}
                    <br />
                    on the waitlist.
                </h2>
                <p style={{ marginTop: 50 }}>
                    You’re registered as {waitlistUser.user_id}. When the
                    countdown reaches zero, we'll invite people to try Fractal.
                    You can signficantly increase your chances of being selected
                    by by{" "}
                    <strong>referring friends and racking up points</strong>.
                </p>
            </div>
        )

    return (
        <div className="fractalContainer">
            <Header color="#111111" />
            <Row style={{ width: "100%", margin: 0, maxWidth: "none" }}>
                <Col md={12}>
                    <div
                        style={{
                            paddingTop: width > 720 ? 100 : 25,
                        }}
                    >
                        {form}
                        <div
                            style={{
                                display: width > 720 ? "flex" : "block",
                                marginTop: 35,
                                maxWidth: "100%",
                            }}
                        >
                            <Button
                                style={{
                                    marginRight: width > 720 ? 25 : 0,
                                    marginBottom: 10,
                                }}
                                className="application-button"
                                onClick={() => {
                                    if (flowState <= 3) {
                                        setFlowState(flowState + 1)
                                    } else {
                                        submitForm()
                                    }
                                }}
                            >
                                {flowState > 3 ? "Submit" : "Continue"}{" "}
                                Application{" "}
                                {flowState > 3
                                    ? ""
                                    : "(" + flowState.toString() + "/3)"}
                            </Button>
                        </div>
                    </div>
                </Col>
            </Row>
        </div>
    )
}

function mapStateToProps(state: { WaitlistReducer: { waitlistUser: any } }) {
    return {
        waitlistUser: state.WaitlistReducer.waitlistUser,
    }
}

export default connect(mapStateToProps)(Application)
