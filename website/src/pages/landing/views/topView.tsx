import React, { useContext } from "react"
import TypeWriterEffect from "react-typewriter-effect"

import WaitlistForm from "pages/landing/components/waitlistForm"
import Header from "shared/components/header"
import ScreenContext from "shared/context/screenContext"

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg"
import Moon from "assets/largeGraphics/moon.svg"
import Mars from "assets/largeGraphics/mars.svg"
import Mercury from "assets/largeGraphics/mercury.svg"
import Saturn from "assets/largeGraphics/saturn.svg"
import Plants from "assets/largeGraphics/plants.svg"
import Mountain from "assets/largeGraphics/mountain.svg"

import "styles/landing.css"

const TopView = (props: any) => {
    const { width } = useContext(ScreenContext)

    return (
        <div
            className="banner-background"
            style={{
                width: "100vw",
                position: "relative",
                zIndex: 1,
                paddingBottom: width > 720 ? 0 : 70,
            }}
        >
            <Header />
            <img
                src={Moon}
                alt=""
                style={{
                    position: "absolute",
                    width: width > 720 ? 100 : 40,
                    height: width > 720 ? 100 : 40,
                    top: 120,
                    left: -20,
                    zIndex: 1,
                }}
            />
            <img
                src={Mars}
                alt=""
                style={{
                    position: "absolute",
                    width: width > 720 ? 100 : 30,
                    height: width > 720 ? 100 : 30,
                    top: 155,
                    right: 0,
                    zIndex: 1,
                }}
            />
            <img
                src={Mercury}
                alt=""
                style={{
                    position: "absolute",
                    width: width > 720 ? 100 : 30,
                    height: width > 720 ? 100 : 30,
                    top: 350,
                    right: 90,
                    zIndex: 1,
                    opacity: width > 720 ? 1.0 : 0,
                }}
            />
            <img
                src={Saturn}
                alt=""
                style={{
                    position: "absolute",
                    width: width > 720 ? 100 : 30,
                    height: width > 720 ? 100 : 30,
                    top: 425,
                    left: 80,
                    zIndex: 1,
                    opacity: width > 720 ? 1.0 : 0,
                }}
            />
            <img
                src={Mountain}
                alt=""
                style={{
                    position: "absolute",
                    width: "100vw",
                    bottom: 150,
                    left: 0,
                    zIndex: 1,
                }}
            />
            <div
                style={{
                    position: "absolute",
                    width: "100vw",
                    bottom: 0,
                    left: 0,
                    height: width > 720 ? 150 : 0,
                    background: "white",
                }}
            ></div>
            <img
                src={Plants}
                alt=""
                style={{
                    position: "absolute",
                    width: width > 720 ? 250 : 100,
                    left: 0,
                    bottom: 130,
                    zIndex: 2,
                }}
            />
            <img
                src={Plants}
                alt=""
                style={{
                    position: "absolute",
                    width: width > 720 ? 250 : 100,
                    right: 0,
                    bottom: 130,
                    transform: "scaleX(-1)",
                    zIndex: 2,
                }}
            />
            <div
                style={{
                    margin: "auto",
                    marginTop: 20,
                    marginBottom: 20,
                    color: "white",
                    textAlign: "center",
                }}
            >
                <div
                    style={{
                        display: "inline-block",
                        margin: "auto",
                        justifyContent: "center",
                        textAlign: "center",
                        paddingLeft: 20,
                        paddingRight: 20,
                    }}
                >
                    <div
                        style={{
                            display: "inline-block",
                            zIndex: 100,
                        }}
                    >
                        <TypeWriterEffect
                            textStyle={{
                                fontFamily: "Maven Pro",
                                color: "#00D4FF",
                                fontSize: "calc(32px + 2.2vw)",
                                fontWeight: "bold",
                                marginTop: width > 720 ? 10 : 7,
                                zIndex: 100,
                                display: "inline-block",
                            }}
                            startDelay={0}
                            cursorColor="white"
                            multiText={[
                                "Blender,",
                                "Figma,",
                                "VSCode,",
                                "Maya,",
                            ]}
                            loop={true}
                            typeSpeed={200}
                        />
                    </div>
                    <div
                        style={{
                            fontSize: "calc(32px + 2.2vw)",
                            fontWeight: "bold",
                            display: "inline-block",
                            paddingBottom: 40,
                            zIndex: 100,
                        }}
                    >
                        &nbsp;just{" "}
                        <span style={{ color: "#00D4FF" }}>faster</span>.
                    </div>
                </div>
                <p
                    style={{
                        width: 650,
                        maxWidth: "100%",
                        margin: "auto",
                        lineHeight: 1.6,
                        color: "#EFEFEF",
                        letterSpacing: 1.5,
                        paddingLeft: 20,
                        paddingRight: 20,
                    }}
                >
                    Fractal uses cloud streaming to supercharge your laptop's
                    applications. Say goodbye to laggy apps — join our waitlist
                    before the countdown ends for access.
                </p>
                <div style={{ marginTop: width > 720 ? 70 : 20, zIndex: 100 }}>
                    <WaitlistForm />
                </div>
            </div>
            <div
                style={{
                    margin: "auto",
                    maxWidth: 1000,
                    width: width > 720 ? "45vw" : "90vw",
                    position: "relative",
                    bottom: width > 720 ? 60 : 30,
                    zIndex: 90,
                    textAlign: "center",
                    pointerEvents: "none",
                }}
            >
                <img
                    src={LaptopAwe}
                    alt=""
                    style={{
                        maxWidth: 1000,
                        width: "100%",
                        zIndex: 90,
                        pointerEvents: "none",
                    }}
                />
            </div>
        </div>
    )
}

export default TopView
