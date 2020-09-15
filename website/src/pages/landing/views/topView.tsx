import React from "react"
import TypeWriterEffect from "react-typewriter-effect"

import WaitlistForm from "pages/landing/components/waitlistForm"
import CountdownTimer from "pages/landing/components/countdown"

import LaptopAwe from "assets/largeGraphics/laptopAwe.svg"
import Moon from "assets/largeGraphics/moon.svg"
import Mars from "assets/largeGraphics/mars.svg"
import Mercury from "assets/largeGraphics/mercury.svg"
import Saturn from "assets/largeGraphics/saturn.svg"
import Plants from "assets/largeGraphics/plants.svg"
import Mountain from "assets/largeGraphics/mountain.svg"

import "styles/landing.css"

function TopView(props: any) {
    return (
        <div
            className="banner-background"
            style={{ width: "100vw", position: "relative", zIndex: 1 }}
        >
            <img
                src={Moon}
                alt=""
                style={{
                    position: "absolute",
                    width: 100,
                    height: 100,
                    top: 125,
                    left: 40,
                }}
            />
            <img
                src={Mars}
                alt=""
                style={{
                    position: "absolute",
                    width: 120,
                    height: 120,
                    top: 185,
                    right: -40,
                }}
            />
            <img
                src={Mercury}
                alt=""
                style={{
                    position: "absolute",
                    width: 80,
                    height: 80,
                    top: 405,
                    right: 90,
                }}
            />
            <img
                src={Saturn}
                alt=""
                style={{
                    position: "absolute",
                    width: 100,
                    height: 100,
                    top: 425,
                    left: 80,
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
                    height: 150,
                    background: "white",
                }}
            ></div>
            <img
                src={Plants}
                alt=""
                style={{
                    position: "absolute",
                    width: 250,
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
                    width: 250,
                    right: 0,
                    bottom: 130,
                    transform: "scaleX(-1)",
                    zIndex: 2,
                }}
            />
            <div
                style={{
                    display: "flex",
                    justifyContent: "space-between",
                    width: "100%",
                    padding: 30,
                }}
            >
                <div className="logo">Fractal</div>
                <div style={{ position: "relative", right: 50 }}>
                    <CountdownTimer />
                </div>
            </div>
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
                        display: "flex",
                        margin: "auto",
                        justifyContent: "center",
                    }}
                >
                    <TypeWriterEffect
                        textStyle={{
                            fontFamily: "Maven Pro",
                            color: "#00D4FF",
                            fontSize: "calc(32px + 2.2vw)",
                            fontWeight: "bold",
                            marginTop: 10,
                        }}
                        startDelay={0}
                        cursorColor="white"
                        multiText={["Blender", "Figma", "VSCode", "Maya"]}
                        loop={true}
                        typeSpeed={200}
                    />
                    <div
                        style={{
                            fontSize: "calc(32px + 2.2vw)",
                            fontWeight: "bold",
                            paddingBottom: 40,
                        }}
                    >
                        , just <span style={{ color: "#00D4FF" }}>faster</span>.
                    </div>
                </div>
                <p
                    style={{
                        width: 650,
                        margin: "auto",
                        lineHeight: 1.6,
                        color: "#EFEFEF",
                        letterSpacing: 1.5,
                    }}
                >
                    Fractal uses cloud streaming to supercharge your laptop's
                    applications. Say goodbye to laggy apps — join our waitlist
                    before the countdown ends for access.
                </p>
                <div style={{ marginTop: 70 }}>
                    <WaitlistForm />
                </div>
            </div>
            <div
                style={{
                    margin: "auto",
                    maxWidth: 1000,
                    width: "45vw",
                    position: "relative",
                    bottom: 60,
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
