import React, { useState, useContext } from "react"
import { Row, Col, Button, FormControl } from "react-bootstrap"
import { connect } from "react-redux"
import { HashLink } from "react-router-hash-link"

import history from "shared/utils/history"
import { db } from "shared/utils/firebase"

import "styles/application.css"

import MainContext from "shared/context/mainContext"
import Header from "shared/components/header"

function Application(props: any) {
    const { user } = props
    const { width } = useContext(MainContext)

    const [devices, setDevices] = useState("")
    const [apps, setApps] = useState("")
    const [source, setSource] = useState("")

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
            .doc(user.email)
            .set({
                devices: devices,
                apps: apps,
                source: source,
            })
            .then(() => history.push("/"))
    }

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
                        <h2>
                            Congrats, {user.name}! You're <br />
                            <span
                                style={{
                                    color: "#3930b8",
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
                            countdown reaches zero, we'll invite people to try
                            Fractal. You can signficantly increase your chances
                            of being selected by{" "}
                            <strong>filling out the form below</strong> below
                            and by{" "}
                            <strong>
                                referring friends and racking up points
                            </strong>
                            .
                        </p>
                        <div className="application-form">
                            <FormControl
                                as="textarea"
                                onChange={changeApps}
                                rows={3}
                                placeholder="Tell us why you're excited to try Fractal! What are the top three apps you'd like us to stream to you (e.g. Chrome, Figma, Blender, Steam, etc.)?"
                                style={{
                                    resize: "none",
                                    outline: "none",
                                    background: "rgba(213, 217, 253, 0.2)",
                                    border: "none",
                                    marginTop: 25,
                                    padding: "20px 30px",
                                }}
                            />
                            <FormControl
                                as="textarea"
                                onChange={changeDevices}
                                rows={3}
                                placeholder="What devices would you use Fractal on (e.g. Windows PC, Mac, Chromebook, tablet, etc.)?"
                                style={{
                                    resize: "none",
                                    outline: "none",
                                    background: "rgba(213, 217, 253, 0.2)",
                                    border: "none",
                                    marginTop: 25,
                                    padding: "20px 30px",
                                }}
                            />
                            <FormControl
                                as="textarea"
                                onChange={changeSource}
                                rows={3}
                                placeholder="How did you hear about us? Please be as specific as possible! (For instance, if you found us through a Google search, what keywords were you searching for?)"
                                style={{
                                    resize: "none",
                                    outline: "none",
                                    background: "rgba(213, 217, 253, 0.2)",
                                    border: "none",
                                    marginTop: 25,
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
                            <HashLink to="/#leaderboard">
                                <button className="white-button">
                                    Back to Home
                                </button>
                            </HashLink>
                        </div>
                    </div>
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
