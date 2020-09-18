import React, { useContext } from "react"
import { Row, Col, Button } from "react-bootstrap"

import ScreenContext from "shared/context/screenContext"

import Testimonial from "pages/landing/components/testimonial"
import Wireframe from "assets/gifs/blenderDemo.gif"

import "styles/landing.css"

function MiddleView(props: any) {
    const { width } = useContext(ScreenContext)
    const scrollToTop = () => {
        window.scrollTo({
            top: 0,
            behavior: "smooth",
        })
    }

    return (
        <div style={{ position: "relative" }}>
            <div style={{ position: "relative", background: "white" }}>
                <Row
                    style={{
                        paddingTop: 50,
                        paddingRight: 50,
                        paddingLeft: 30,
                    }}
                >
                    <Col
                        md={7}
                        xs={12}
                        style={{
                            paddingRight: width > 720 ? 50 : 0,
                        }}
                    >
                        <img
                            src={Wireframe}
                            alt=""
                            style={{
                                width: "100%",
                                borderRadius: 5,
                                boxShadow: "0px 4px 30px rgba(0, 0, 0, 0.15)",
                            }}
                        />
                    </Col>
                    <Col md={5} style={{ paddingLeft: width > 720 ? 45 : 15 }}>
                        <h2
                            style={{
                                fontWeight: "bold",
                                lineHeight: 1.2,
                                marginTop: width > 720 ? 0 : 25,
                            }}
                        >
                            Run your favorite apps on any device.
                        </h2>
                        <p style={{ marginTop: 40 }}>
                            Fractal is like Google Stadia for creative and
                            productivity apps. With Fractal, you can use VSCode
                            on an iPad or 3D animation software on a Chromebook,
                            all while consuming 10x less RAM and processing
                            power.
                        </p>
                        <Button className="access-button" onClick={scrollToTop}>
                            REQUEST ACCESS
                        </Button>
                    </Col>
                </Row>
            </div>
            <div
                style={{
                    padding: 30,
                    marginTop: width > 720 ? 100 : 40,
                    paddingRight: width > 720 ? 45 : 30,
                }}
            >
                <Row>
                    <Col md={6} style={{ paddingLeft: 15 }}>
                        <div
                            style={{
                                padding: "50px 35px",
                                display: "flex",
                                justifyContent: "space-between",
                                background: "rgba(221, 165, 248, 0.2)",
                                borderRadius: 5,
                                marginBottom: 25,
                            }}
                        >
                            <h2
                                style={{
                                    fontWeight: "bold",
                                    lineHeight: 1.3,
                                }}
                            >
                                Q:
                            </h2>
                            <div style={{ width: 35 }}></div>
                            <div>
                                <h2
                                    style={{
                                        fontWeight: "bold",
                                        lineHeight: 1.3,
                                        paddingBottom: 10,
                                    }}
                                >
                                    How can I be invited to try Fractal?
                                </h2>
                                <p style={{ paddingTop: 20 }}>
                                    When the countdown hits zero, we’ll invite{" "}
                                    <strong>20 people</strong> from the waitlist
                                    with the most compelling{" "}
                                    <strong>100-word submission</strong> on why
                                    they want Fractal to receive 1:1 onboarding.
                                    We'll also invite the{" "}
                                    <strong>top 20</strong> people on the
                                    leaderboard, which you can climb by
                                    referring friends.
                                </p>
                                <Button
                                    className="access-button"
                                    onClick={scrollToTop}
                                >
                                    JOIN NOW
                                </Button>
                            </div>
                        </div>
                        <Testimonial
                            text="F*ING AMAZING ... tried Minecraft VR - it was buttery smooth with almost no detectable latency. Never thought it would work this well."
                            title="Sean S."
                            subtitle="Designer + VR user"
                        />
                    </Col>
                    <Col md={6}>
                        <Testimonial
                            text="Woah! What you've developed is something I've been dreaming of for years now... this is a MASSIVE game changer for people that do the kind of work that I do."
                            title="Brian M."
                            subtitle="Animator"
                        />
                        <Testimonial
                            text="[Fractal] is an immense project with a fantastic amount of potential."
                            title="Jonathan H."
                            subtitle="Software developer + gamer"
                        />
                    </Col>
                </Row>
            </div>
        </div>
    )
}

export default MiddleView
