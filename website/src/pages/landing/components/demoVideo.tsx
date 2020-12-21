import React from "react"
import { Row, Col } from "react-bootstrap"

import AnimatedComputer from "assets/icons/animatedComputer.svg"
import AnimatedRouter from "assets/icons/animatedRouter.svg"
import AnimatedCloud from "assets/icons/animatedCloud.svg"
import DemoBackground from "assets/largeGraphics/demoBackground.svg"
// import BlenderShadow from "assets/largeGraphics/blenderShadow.svg"
import PerformanceChartAfter from "assets/largeGraphics/performanceChartAfter.svg"
import PerformanceChartBefore from "assets/largeGraphics/performanceChartBefore.svg"

import BlenderDemo from "assets/gifs/blenderDemo.gif"

const DemoVideo = (props: any) => {
    const { width } = props

    return (
        <div style={{ position: "relative", width: "100%", marginTop: 75 }}>
            <div style={{ height: width > 1400 ? 75 : 0 }}></div>
            <div style={{ paddingTop: width > 720 ? 125 : 0 }}>
                <h2
                    style={{
                        fontSize: 50,
                        fontWeight: "bold",
                        color: "#111111",
                        lineHeight: 1.3,
                    }}
                >
                    No GPU? No Problem.
                </h2>
                <p style={{ marginTop: 20, maxWidth: 1000 }}>
                    Fractal runs your apps on cloud GPUs. Unlike virtual
                    desktops, Fractal streams you just the app, so apps feel
                    like they’re on your own computer.
                </p>
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
                        src={AnimatedComputer}
                        style={{ width: 30, height: 30 }}
                        alt=""
                    />
                    <div style={{ marginLeft: 20, maxWidth: 220 }}>
                        60+ frames per second
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
                        src={AnimatedRouter}
                        style={{
                            width: 30,
                            height: 30,
                            position: "relative",
                            bottom: 5,
                        }}
                        alt=""
                    />
                    <div style={{ marginLeft: 20 }}>
                        Full encryption and data privacy
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
                        marginBottom: 15,
                        display: "flex",
                    }}
                >
                    <img
                        src={AnimatedCloud}
                        style={{ width: 30, height: 30 }}
                        alt=""
                    />
                    <div style={{ marginLeft: 20 }}>
                        Automatic file upload and download
                    </div>
                </Col>
            </Row>
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
        </div>
    )
}

export default DemoVideo
