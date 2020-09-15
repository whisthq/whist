import React, { useState } from "react"
import { Row, Col, Button, FormControl } from "react-bootstrap"
import { connect } from "react-redux"
import { Link } from "react-router-dom"

import history from "utils/history"
import { db } from "utils/firebase"

import "styles/application.css"

import PaintingSky from "assets/largeGraphics/paintingSky.svg"

function Application(props: any) {
    const { user } = props

    const [form, setForm] = useState("")

    function changeForm(evt: any) {
        evt.persist()
        setForm(evt.target.value)
    }

    function submitForm() {
        db.collection("waitlist")
            .doc(user.email)
            .update({
                form: form,
            })
            .then(() => history.push("/"))
    }

    return (
        <div>
            <Row style={{ width: "100%", margin: 0, maxWidth: "none" }}>
                <Col md={7}>
                    <div
                        style={{
                            padding: 50,
                            paddingTop: 100,
                        }}
                    >
                        <h2>
                            Congrats, Ming! You're <br />
                            <span
                                style={{
                                    color: "#00d4ff",
                                    fontSize: "calc(140px + 2.2vw)",
                                }}
                            >
                                No. 1,902
                            </span>{" "}
                            <br />
                            on the waitlist.
                        </h2>
                        <p style={{ marginTop: 50 }}>
                            You’re on the waitlist as ming@fractalcomputers.com.
                            When the countdown reaches zero, we'll invite people
                            to receive 1:1 onboarding. You can signficantly
                            increase your chances of being selected by{" "}
                            <strong>
                                telling us why you should be selected
                            </strong>{" "}
                            below, or by{" "}
                            <strong>
                                referring friends and racking up points
                            </strong>
                            .
                        </p>
                        <div className="application-form">
                            <FormControl
                                as="textarea"
                                onChange={changeForm}
                                rows={8}
                                placeholder="Tell us why you're excited to try Fractal!"
                                style={{
                                    resize: "none",
                                    outline: "none",
                                    background: "rgba(213, 217, 253, 0.4)",
                                    border: "none",
                                    marginTop: 50,
                                    padding: "20px 30px",
                                }}
                            />
                        </div>
                        <div style={{ display: "flex", marginTop: 25 }}>
                            <Button
                                style={{ marginRight: 25 }}
                                className="application-button"
                                onClick={submitForm}
                            >
                                Submit Application
                            </Button>
                            <Link to="/">
                                <Button
                                    className="application-button"
                                    style={{ backgroundColor: "#322FA8" }}
                                >
                                    Back to Home
                                </Button>
                            </Link>
                        </div>
                    </div>
                </Col>
                <Col
                    md={5}
                    style={{ textAlign: "right", margin: 0, padding: 0 }}
                >
                    <img
                        src={PaintingSky}
                        alt=""
                        style={{ width: "100%", maxWidth: 900 }}
                    />
                </Col>
            </Row>
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Application)
