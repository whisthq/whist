import React, { useState, useContext } from "react"
import { Row, Col, Button, FormControl } from "react-bootstrap"
import { connect } from "react-redux"
import { Link } from "react-router-dom"

import history from "shared/utils/history"
import { db } from "shared/utils/firebase"

import "styles/application.css"

import PaintingSky from "assets/largeGraphics/paintingSky.svg"

import ScreenContext from "shared/context/screenContext"
import Header from "shared/components/header"

const Application = (props: any) => {
    const { user } = props
    const { width } = useContext(ScreenContext)

    const [form, setForm] = useState("")

    const changeForm = (evt: any) => {
        evt.persist()
        setForm(evt.target.value)
    }

    const submitForm = () => {
        db.collection("waitlist")
            .doc(user.email)
            .update({
                form: form,
            })
            .then(() => history.push("/"))
    }

    return (
        <div>
            <Header color="#111111" />
            <Row style={{ width: "100%", margin: 0, maxWidth: "none" }}>
                <Col md={7}>
                    <div
                        style={{
                            padding: 15,
                            paddingTop: width > 720 ? 100 : 25,
                        }}
                    >
                        <h2>
                            Congrats, {user.name}! You're <br />
                            <span
                                style={{
                                    color: "#00d4ff",
                                    fontSize:
                                        width > 720
                                            ? "calc(140px + 2.2vw)"
                                            : 50,
                                }}
                            >
                                No. {user.ranking}
                            </span>{" "}
                            <br />
                            on the waitlist.
                        </h2>
                        <p style={{ marginTop: 50 }}>
                            You’re registered as {user.email}. When the
                            countdown reaches zero, we'll invite people to
                            receive 1:1 onboarding. You can signficantly
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
                        <div
                            style={{
                                display: width > 720 ? "flex" : "block",
                                marginTop: 25,
                                maxWidth: "100%",
                            }}
                        >
                            <Button
                                style={{
                                    marginRight: width > 720 ? 25 : 0,
                                    marginBottom: 10,
                                }}
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
                {width > 720 && (
                    <Col
                        md={5}
                        style={{
                            textAlign: "right",
                            margin: 0,
                            padding: 0,
                        }}
                    >
                        <img
                            src={PaintingSky}
                            alt=""
                            style={{ width: "100%", maxWidth: 900 }}
                        />
                    </Col>
                )}
            </Row>
        </div>
    )
}

const mapStateToProps = (state: { MainReducer: { user: any } }) => {
    return {
        user: state.MainReducer.user,
    }
}

export default connect(mapStateToProps)(Application)
