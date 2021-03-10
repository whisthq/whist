import React, { useContext } from "react"
import { Row, Col } from "react-bootstrap"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"

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

export const SideBySide = (props: any) => {
    const { appHighlight, width } = useContext(MainContext)

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
            <span>
                <span>
                    Fractal leverages our high-fidelity streaming tech to run
                    and stream your app from the nearest AWS server.
                </span>
                <span style={{ marginTop: 20, display: "block" }}>
                    Using an app on Fractal is as easy as using the app
                    normally. We handle all the complexity so that your
                    experience is seamless. All that's required to run Fractal
                    is an 8 Mbps Internet connection.
                </span>
            </span>
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
                    position: "relative",
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
                    transform: "translate(-50%, -50%)",
                    width: "50%",
                    display: "inline-block",
                }}
                src={SpeedTest}
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: width > ScreenSize.SMALL ? "40px" : "20px",
                    right: width > ScreenSize.SMALL ? "-40px" : "-20px",
                    width: width > ScreenSize.SMALL ? "100px" : "60px",
                    animationDelay: "0.2s",
                    display: "inline-block",
                }}
                src={ChromeShadow}
                className="animate-bounce"
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: width > ScreenSize.SMALL ? "140px" : "90px",
                    right: width > ScreenSize.SMALL ? "-40px" : "-20px",
                    width: width > ScreenSize.SMALL ? "100px" : "60px",
                    display: "inline-block",
                }}
                src={FirefoxShadow}
                className="animate-bounce"
                alt=""
            />
            <img
                style={{
                    position: "absolute",
                    top: width > ScreenSize.SMALL ? "260px" : "170px",
                    right: width > ScreenSize.SMALL ? "-40px" : "-20px",
                    width: width > ScreenSize.SMALL ? "95px" : "60px",
                    animationDelay: "0.4s",
                    display: "inline-block",
                }}
                className="animate-bounce"
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
                bottom: width > ScreenSize.SMALL ? 20 : 50,
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
                        top: width > ScreenSize.SMALL ? "40px" : "20px",
                        left: width > ScreenSize.SMALL ? "-40px" : "-20px",
                        width: width > ScreenSize.SMALL ? "100px" : "60px",
                        animationDelay: "0.2s",
                    }}
                    src={BlenderShadow}
                    className="animate-bounce"
                    alt=""
                />
                <img
                    style={{
                        position: "absolute",
                        top: width > ScreenSize.SMALL ? "140px" : "90px",
                        left: width > ScreenSize.SMALL ? "-40px" : "-20px",
                        width: width > ScreenSize.SMALL ? "100px" : "60px",
                    }}
                    src={MayaShadow}
                    className="animate-bounce"
                    alt=""
                />
                <img
                    style={{
                        position: "absolute",
                        top: width > ScreenSize.SMALL ? "260px" : "170px",
                        left: width > ScreenSize.SMALL ? "-25px" : "-10px",
                        width: width > ScreenSize.SMALL ? "70px" : "40px",
                        animationDelay: "0.4s",
                    }}
                    className="animate-bounce"
                    src={UnityShadow}
                    alt=""
                />
            </div>
        </div>
    )
    const Gaming = (
        <div style={{ marginBottom: width > ScreenSize.LARGE ? 0 : 70 }}>
            <img
                src={GraphicsImage}
                style={{ width: "100%", margin: "auto" }}
                alt=""
            />
            <div
                style={{
                    position: "absolute",
                    zIndex: 1,
                    height: "100%",
                    width: "100%",
                    left: width > ScreenSize.LARGE ? 50 : 25,
                    top: width > ScreenSize.LARGE ? 50 : 25,
                    background: "rgba(213, 225, 245, 0.2)",
                }}
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
                padding:
                    props.case === "Productivity" && width < ScreenSize.MEDIUM
                        ? 0
                        : "50px 0px",
            }}
            id={props.case}
        >
            <Row
                style={{
                    flexDirection:
                        props.case === "Productivity" &&
                        width < ScreenSize.MEDIUM
                            ? "column-reverse"
                            : "row",
                }}
            >
                <Col
                    md={{
                        span: props.case === "Productivity" ? 6 : 12,
                    }}
                    lg={{ span: 6 }}
                >
                    <div style={{ position: "relative" }}>
                        <div>{images[props.case]}</div>
                    </div>
                </Col>
                <Col
                    md={{
                        span: props.case === "Productivity" ? 5 : 12,
                        offset: props.case === "Productivity" ? 1 : 0,
                    }}
                    lg={{ span: 5, offset: 1 }}
                >
                    <div
                        style={{
                            display: "flex",
                            flexDirection: "column",
                            justifyContent: "flex-end",
                            marginBottom:
                                props.case === "Productivity" &&
                                width < ScreenSize.MEDIUM
                                    ? 70
                                    : 0,
                        }}
                    >
                        <h2
                            style={{
                                fontSize: 50,
                                color: "#111111",
                                lineHeight: 1.3,
                            }}
                        >
                            {headers[props.case]}
                        </h2>
                        <p
                            className="font-body"
                            style={{
                                color: "#111111",
                                paddingTop: 30,
                            }}
                        >
                            {descriptions[props.case]}
                        </p>
                        {props.case === "Gaming" && (
                            <div style={{ marginTop: 30 }}></div>
                        )}
                    </div>
                </Col>
            </Row>
        </div>
    )
}

export default SideBySide
