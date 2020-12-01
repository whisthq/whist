import React from "react"
import { Row, Col } from "react-bootstrap"

import AnimatedCheckmark from "assets/icons/animatedCheckmark.svg"
import AnimatedChatbox from "assets/icons/animatedChatbox.svg"
import AnimatedRocket from "assets/icons/animatedRocket.svg"
import AnimatedComputer from "assets/icons/animatedComputer.svg"
import AnimatedRouter from "assets/icons/animatedRouter.svg"
import AnimatedCloud from "assets/icons/animatedCloud.svg"
import DemoBackground from "assets/largeGraphics/demoBackground.svg"
// import BlenderShadow from "assets/largeGraphics/blenderShadow.svg"
import PerformanceChartAfter from "assets/largeGraphics/performanceChartAfter.svg"
import PerformanceChartBefore from "assets/largeGraphics/performanceChartBefore.svg"

import ReactPlayer from "react-player"
import { config } from "shared/constants/config"

import BlenderDemo from "assets/gifs/blenderDemo.gif"

const DemoVideo = (props: any) => {
    // icons should have length 3
    const { width, heading, text, icons, component, first } = props

    return (
        <div
            style={{
                position: "relative",
                width: "100%",
                marginTop: first ? 75 : 25,
            }}
        >
            <div style={{ height: width > 1400 ? 75 : 0 }}></div>
            <div
                style={{
                    paddingTop: first ? (width > 720 ? 125 : 0) : 15,
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
                    {heading}
                </h2>
                <p style={{ marginTop: 20, maxWidth: 1000 }}>{text}</p>
            </div>
            <Row
                style={{
                    width: "100%",
                    maxWidth: 975,
                    marginTop: 40,
                    marginLeft: 0,
                }}
            >
                <Col
                    xs={12}
                    md={4}
                    style={{
                        display: "flex",
                        width: "100%",
                        padding: "20px 20px",
                        background: "rgba(213, 225, 245, 0.2)",
                        fontSize: 14,
                        borderRadius: 5,
                        maxWidth: 275,
                        marginRight: 30,
                        marginBottom: 15,
                    }}
                >
                    <img
                        src={icons[0][0]}
                        style={{ width: 30, height: 30 }}
                        alt=""
                    />
                    <div style={{ marginLeft: 20, maxWidth: 220 }}>
                        {icons[0][1]}
                    </div>
                </Col>
                <Col
                    xs={12}
                    md={4}
                    style={{
                        width: "100%",
                        padding: "20px 20px",
                        background: "rgba(213, 225, 245, 0.2)",
                        fontSize: 14,
                        borderRadius: 5,
                        maxWidth: 275,
                        marginRight: 30,
                        marginBottom: 15,
                        display: "flex",
                    }}
                >
                    <img
                        src={icons[1][0]}
                        style={{
                            width: 30,
                            height: 30,
                            position: "relative",
                            bottom: 5,
                        }}
                        alt=""
                    />
                    <div style={{ marginLeft: 20 }}>{icons[1][1]}</div>
                </Col>
                <Col
                    xs={12}
                    md={4}
                    style={{
                        width: "100%",
                        padding: "20px 20px",
                        background: "rgba(213, 225, 245, 0.2)",
                        fontSize: 14,
                        borderRadius: 5,
                        maxWidth: 275,
                        marginBottom: 15,
                        display: "flex",
                    }}
                >
                    <img
                        src={icons[2][0]}
                        style={{ width: 30, height: 30 }}
                        alt=""
                    />
                    <div style={{ marginLeft: 20 }}>{icons[2][1]}</div>
                </Col>
            </Row>
            {component}
        </div>
    )
}

const DemoVideos = (props: any) => {
    const { width } = props

    const gifHeading = "No GPU? No Problem."
    const gifText =
        "Fractal runs your apps on cloud GPUs. Unlike virtual desktops, Fractal streams you just the app, so apps feel like they’re on your own computer."
    const gifIcons = [
        [AnimatedComputer, "60+ frames per second"],
        [AnimatedRouter, "Full encryption and data privacy"],
        [AnimatedCloud, "Automatic file upload and download"],
    ]

    const gifComponent = (
        <div
            style={{
                width: "100%",
                position: "relative",
                marginTop: 40,
            }}
        >
            <div>
                <img
                    src={DemoBackground}
                    alt=""
                    style={{
                        width:
                            width > 700
                                ? "calc(100% - 100px)"
                                : "calc(100% - 40px)",
                        boxShadow: "0px 4px 20px rgba(0,0,0,0.15)",
                        borderRadius: "5px 5px 0px 0px",
                        maxWidth: 1350,
                    }}
                />
                <img
                    style={{
                        width:
                            width > 700
                                ? "calc(100% - 100px)"
                                : "calc(100% - 40px)",
                        maxHeight: "600px",
                        boxShadow: "0px 4px 20px rgba(0,0,0,0.25)",
                        borderRadius: "0px 0px 5px 5px",
                        maxWidth: 1350,
                        height: "auto",
                    }}
                    src={BlenderDemo}
                    alt=""
                />
            </div>
            {/* <img
                src={BlenderShadow}
                style={{
                    position: "absolute",
                    top: 20,
                    right: -50,
                    width: 120,
                    height: 120,
                    animationDelay: "0.2s",
                }}
                className="bounce"
                alt=""
            /> */}
            <img
                src={PerformanceChartAfter}
                style={{
                    position: "absolute",
                    bottom: -40,
                    right: 0,
                    width: width > 775 ? 250 : 100,
                    borderRadius: 5,
                    boxShadow: "0px 4px 20px rgba(0,0,0,0.1)",
                    animationDelay: "4s",
                }}
                className="animate-flicker"
                alt=""
            />
            <img
                src={PerformanceChartBefore}
                style={{
                    position: "absolute",
                    bottom: -40,
                    right: 0,
                    width: width > 775 ? 250 : 100,
                    borderRadius: 5,
                    boxShadow: "0px 4px 20px rgba(0,0,0,0.1)",
                    animationDelay: "4s",
                }}
                className="animate-flicker-alt"
                alt=""
            />
        </div>
    )

    const demoHeading = "How do I use it?"
    const demoText =
        "Curious to see what using Fractal feels like? Watch it's performance side by side with that of native apps and learn from our team about how to seamlessly integrate it with your workflow."
    const demoIcons = [
        [AnimatedCheckmark, "Run your graphical application seamlessly"],
        [AnimatedChatbox, "Recieve support from our team and community"],
        [AnimatedRocket, "Supercharge your computer"],
    ]

    // huh?!
    const demoComponent = (
        <div
            style={{
                width: "100%",
                paddingTop: "56.25%",
                position: "relative",
                marginTop: 40,
                marginLeft: 0,
            }}
        >
            <ReactPlayer
                style={{
                    position: "absolute",
                    top: 0,
                    left: 0,
                    bottom: 0,
                    right: 0,
                }}
                width="100%"
                height="100%"
                url="https://www.youtube.com/watch?v=LXb3EKWsInQ" //subject to change/ should be hd
                controls
            />
        </div>
    )

    return (
        <div>
            <DemoVideo
                width={width}
                heading={gifHeading}
                text={gifText}
                icons={gifIcons}
                component={gifComponent}
                first
            />
            {config.video_ready && (
                <DemoVideo
                    width={width}
                    heading={demoHeading}
                    text={demoText}
                    icons={demoIcons}
                    component={demoComponent}
                />
            )}
        </div>
    )
}

export default DemoVideos
