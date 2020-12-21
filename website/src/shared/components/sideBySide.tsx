import React, { useContext } from "react"
import { Row, Col } from "react-bootstrap"

import WaitlistForm from "shared/components/waitlistForm"
import MainContext from "shared/context/mainContext"

import SecretPoints from "shared/components/secretPoints"
import {
    SECRET_POINTS,
    EASTEREGG_POINTS,
    EASTEREGG_RAND,
} from "shared/utils/points"

import GraphicsImage from "assets/largeGraphics/graphics.svg"
import SpeedTestBackground from "assets/largeGraphics/speedTestBackground.svg"
import SpeedTest from "assets/gifs/speedTest.gif"
import DemoBackground from "assets/largeGraphics/demoBackground.svg"
import BlenderDemo from "assets/gifs/blenderDemo.gif"
import ChromeShadow from "assets/largeGraphics/chromeShadow.svg"
import FirefoxShadow from "assets/largeGraphics/firefoxShadow.svg"
import FigmaShadow from "assets/largeGraphics/figmaShadow.svg"
import BlenderShadow from "assets/largeGraphics/blenderShadow.svg"
import MayaShadow from "assets/largeGraphics/mayaShadow.svg"
import UnityShadow from "assets/largeGraphics/unityShadow.svg"

function SideBySide(props: any) {
    const { appHighlight } = useContext(MainContext)

    const yourApps = appHighlight ? appHighlight : "your apps"
    const yourApplications = appHighlight ? appHighlight : "your apps"
    const theyUse = appHighlight ? "it uses" : "they use"
    const access = appHighlight ? "accesses" : "access"
    const theApp = appHighlight ? appHighlight : "your app"

    const descriptions: any = {
        Productivity:
            "When you run " +
            yourApps +
            " through Fractal, " +
            theyUse +
            " 10x less RAM and " +
            access +
            " gigabit Internet speeds.",
        Graphics: (
            <div>
                <div>
                    Fractal runs {yourApplications} on cloud GPUs. Unlike
                    virtual desktops, Fractal streams just the app, so using{" "}
                    {theApp} feels native.
                </div>
            </div>
        ),
        Gaming: (
            <div>
                <div>
                    Fractal leverages our high-fidelity streaming tech to run
                    and stream your app from the nearest AWS server.
                </div>
                <div style={{ marginTop: 20 }}>
                    Using an app on Fractal is as easy as using the app
                    normally. We handle all the complexity so that your
                    experience is seamless. All that's required to run Fractal
                    is an 8 Mbps Internet connection.
                </div>
            </div>
        ),
    }

    const Productivity = (
        <div
            style={{
                width: "100%",
                margin: "auto",
                position: "relative",
                bottom: 40,
            }}
        >
            <img
                style={{
                    position: "absolute",
                    top: 0,
                    left: 0,
                    width: "100%",
                    display: "inline-block",
                }}
                src={SpeedTestBackground}
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: "50%",
                    left: "50%",
                    transform: "translate(-53%, 25%)",
                    width: "50%",
                    display: "inline-block",
                }}
                src={SpeedTest}
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: props.width < 700 ? "20px" : "40px",
                    right: props.width < 700 ? "-20px" : "-40px",
                    width: props.width < 700 ? "60px" : "100px",
                    animationDelay: "0.2s",
                    display: "inline-block",
                }}
                src={ChromeShadow}
                className="bounce"
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: props.width < 700 ? "90px" : "140px",
                    right: props.width < 700 ? "-20px" : "-40px",
                    width: props.width < 700 ? "60px" : "100px",
                    display: "inline-block",
                }}
                src={FirefoxShadow}
                className="bounce"
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: props.width < 700 ? "170px" : "260px",
                    right: props.width < 700 ? "-20px" : "-40px",
                    width: props.width < 700 ? "60px" : "95px",
                    animationDelay: "0.4s",
                    display: "inline-block",
                }}
                className="bounce"
                src={FigmaShadow}
                alt=""
            />
        </div>
    )

    const Graphics = (
        <div
            style={{
                width: "100%",
                margin: "auto",
                position: "relative",
                bottom: props.width < 700 ? 50 : 20,
            }}
        >
            <div>
                <img
                    style={{
                        position: "absolute",
                        top: 0,
                        left: 0,
                        width: "100%",
                    }}
                    src={DemoBackground}
                    alt=""
                />
                <img
                    style={{
                        position: "absolute",
                        top: "23px",
                        left: "21px",
                        width: "97%",
                        maxHeight: "475px",
                        boxShadow: "0px 4px 20px rgba(0,0,0,0.25)",
                        borderRadius: "0px 0px 5px 5px",
                    }}
                    src={BlenderDemo}
                    alt=""
                />
                <img
                    style={{
                        position: "absolute",
                        top: props.width < 700 ? "20px" : "40px",
                        left: props.width < 700 ? "-20px" : "-40px",
                        width: props.width < 700 ? "60px" : "100px",
                        animationDelay: "0.2s",
                    }}
                    src={BlenderShadow}
                    className="bounce"
                    alt=""
                />
                <img
                    style={{
                        position: "absolute",
                        top: props.width < 700 ? "90px" : "140px",
                        left: props.width < 700 ? "-20px" : "-40px",
                        width: props.width < 700 ? "60px" : "100px",
                    }}
                    src={MayaShadow}
                    className="bounce"
                    alt=""
                />
                <img
                    style={{
                        position: "absolute",
                        top: props.width < 700 ? "170px" : "260px",
                        left: props.width < 700 ? "-10px" : "-25px",
                        width: props.width < 700 ? "40px" : "70px",
                        animationDelay: "0.4s",
                    }}
                    className="bounce"
                    src={UnityShadow}
                    alt=""
                />
            </div>
        </div>
    )

    const Gaming = (
        <div>
            <img
                src={GraphicsImage}
                style={{ width: "100%", margin: "auto" }}
                alt=""
            />
        </div>
    )

    const images: any = {
        Productivity: Productivity,
        Gaming: Gaming,
        Graphics: Graphics,
    }

    const headers: any = {
        Productivity: "Unlock more RAM and blazing-fast Internet.",
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
                        span: props.reverse ? 7 : 6,
                        order: props.reverse ? 2 : 1,
                    }}
                >
                    <div style={{ position: "relative" }}>
                        <div>{images[props.case]}</div>
                        <div
                            style={{
                                position: "absolute",
                                zIndex: 1,
                                height: props.width > 720 ? "100%" : "50%",
                                width: "100%",
                                maxWidth: 800,
                                left: props.reverse ? -50 : 50,
                                top: 50,
                                background: "rgba(213, 225, 245, 0.2)",
                            }}
                        />
                    </div>
                </Col>
                <Col
                    md={{
                        span: props.reverse ? 5 : 6,
                        order: props.reverse ? 1 : 2,
                    }}
                >
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            justifyContent: "flex-end",
                            paddingLeft:
                                props.reverse || props.width < 775 ? 0 : 100,
                            paddingRight:
                                props.reverse && props.width > 775 ? 100 : 0,
                            marginTop: props.width < 775 ? "55vw" : 0,
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
                        {props.case === "Gaming" ||
                        props.case === "Graphics" ? (
                            <SecretPoints
                                points={EASTEREGG_POINTS + EASTEREGG_RAND()}
                                name={
                                    props.case === "Graphics"
                                        ? SECRET_POINTS.LANDING_NO_GPU_NO_PROBLEM
                                        : SECRET_POINTS.ABOUT_HOW_IT_WORKS
                                }
                            />
                        ) : (
                            <div />
                        )}
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
