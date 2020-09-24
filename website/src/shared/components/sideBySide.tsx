import React, { useContext } from "react"
import { Row, Col } from "react-bootstrap"

import WaitlistForm from "shared/components/waitlistForm"
import MainContext from "shared/context/mainContext"

import Gaming from "assets/largeGraphics/gaming.svg"
import Graphics from "assets/largeGraphics/graphics.svg"
import Productivity from "assets/largeGraphics/productivity.svg"

function SideBySide(props: any) {
    const { appHighlight } = useContext(MainContext)

    const yourApps = appHighlight ? appHighlight : "your apps"
    const yourApplications = appHighlight ? appHighlight : "your applications"
    const theyUse = appHighlight ? "it uses" : "they use"
    const access = appHighlight ? "accesses" : "access"
    const theApp = appHighlight ? appHighlight : "the app"

    const descriptions: any = {
        Productivity:
            "Fractal is like Google Stadia for your creative and productivity apps. When you run " +
            yourApps +
            " through Fractal, " +
            theyUse +
            " 10x less RAM and " +
            access +
            " gigabyte Internet speeds.",
        Graphics:
            "Fractal runs " +
            yourApplications +
            " on dedicated cloud GPUs and streams them to you at 60FPS, similar to how Netflix streams you movies. Unlike traditional virtual desktops, Fractal streams you just the application, so using " +
            theApp +
            " feels completely native.",
        Gaming: (
            <div>
                <div>
                    Fractal runs and streams your app from the nearest AWS
                    server by leveraging our high-fidelity streaming.
                </div>
                <div style={{ marginTop: 20 }}>
                    Using an app on Fractal is as easy as using the application
                    normally. We handle all the complexity so that your
                    experience is seamless. All that's required to run Fractal
                    is an 8 Mbps Internet connection.
                </div>
            </div>
        ),
    }

    const images: any = {
        Productivity: Productivity,
        Gaming: Gaming,
        Graphics: Graphics,
    }

    const headers: any = {
        Productivity:
            "Give " + yourApps + " more RAM and blazing-fast Internet.",
        Gaming: "How It Works",
        Graphics: "No GPU? No problem.",
    }

    return (
        <div
            style={{
                padding: "40px 0px",
            }}
            id={props.case}
        >
            <Row>
                <Col
                    md={{
                        span: 6,
                        order: props.reverse ? 2 : 1,
                    }}
                >
                    <div style={{ position: "relative" }}>
                        <img
                            style={{
                                width: "100%",
                                margin: "auto",
                                borderRadius: 3,
                                boxShadow: "0px 4px 20px rgba(0, 0, 0, 0.2)",
                            }}
                            src={images[props.case]}
                            alt=""
                        />
                        <div
                            style={{
                                position: "absolute",
                                zIndex: 0,
                                height: props.width > 720 ? "100%" : "50%",
                                width: "100%",
                                maxWidth: 800,
                                left: props.reverse ? -50 : 50,
                                top: 50,
                                background: "rgba(213, 225, 245, 0.2)",
                            }}
                        ></div>
                    </div>
                </Col>
                <Col md={{ span: 6, order: props.reverse ? 1 : 2 }}>
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            justifyContent: "flex-end",
                            paddingLeft:
                                props.reverse || props.width < 700 ? 0 : 150,
                            paddingRight:
                                props.reverse && props.width > 700 ? 150 : 0,
                            marginTop: props.width < 700 ? 30 : 0,
                        }}
                    >
                        <h2
                            style={{
                                fontSize: 50,
                                fontWeight: "bold",
                                color: "#111111",
                                lineHeight: 1.3,
                            }}
                        >
                            {headers[props.case]}
                        </h2>
                        <p
                            style={{
                                color: "#111111",
                                paddingTop: 30,
                            }}
                        >
                            {descriptions[props.case]}
                        </p>
                        {props.case === "Gaming" && (
                            <div style={{ marginTop: 30 }}>
                                <WaitlistForm />
                            </div>
                        )}
                    </div>
                </Col>
            </Row>
        </div>
    )
}

export default SideBySide
