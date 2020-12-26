import React, { useState, useContext } from "react"
import TypeWriterEffect from "react-typewriter-effect"
import { Row, Col } from "react-bootstrap"

import Header from "shared/components/header"
import WaitlistForm from "shared/components/waitlistForm"
import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"
import FloatingLogo from "pages/landing/components/floatingLogo"

import BlenderNoColor from "assets/largeGraphics/blenderNoColor.svg"
import UnityNoColor from "assets/largeGraphics/unityNoColor.svg"
import FigmaNoColor from "assets/largeGraphics/figmaNoColor.svg"
import ChromeNoColor from "assets/largeGraphics/chromeNoColor.svg"
import MayaNoColor from "assets/largeGraphics/mayaNoColor.svg"
import FirefoxNoColor from "assets/largeGraphics/firefoxNoColor.svg"
import BlenderColor from "assets/largeGraphics/blenderColor.svg"
import UnityColor from "assets/largeGraphics/unityColor.svg"
import FigmaColor from "assets/largeGraphics/figmaColor.svg"
import ChromeColor from "assets/largeGraphics/chromeColor.svg"
import MayaColor from "assets/largeGraphics/mayaColor.svg"
import FirefoxColor from "assets/largeGraphics/firefoxColor.svg"

import UnityScreenshot from "assets/largeGraphics/unityScreenshot.svg"
import BlenderTiger from "assets/largeGraphics/blenderTiger.svg"
import FigmaScreenshot from "assets/largeGraphics/figmaScreenshot.svg"
import MayaScreenshot from "assets/largeGraphics/mayaScreenshot.svg"
import YoutubeScreenshot from "assets/largeGraphics/youtube.svg"

import "styles/landing.css"

const TopView = (props: any) => {
    const { width, appHighlight } = useContext(MainContext)
    const [currentIndex, setCurrentIndex] = useState(0)

    const apps = [
        "Firefox,",
        "Blender,",
        "Figma,",
        "Unity,",
        "Chrome,",
        "Maya,",
        "Your apps,",
    ]

    const appImages = [
        {
            name: "Firefox",
            colorImage: FirefoxColor,
            noColorImage: FirefoxNoColor,
            rgb: "0,0,0",
        },
        {
            name: "Blender",
            colorImage: BlenderColor,
            noColorImage: BlenderNoColor,
            rgb: "0,0,0",
        },
        {
            name: "Figma",
            colorImage: FigmaColor,
            noColorImage: FigmaNoColor,
            rgb: "0,0,0",
        },
        {
            name: "Unity",
            colorImage: UnityColor,
            noColorImage: UnityNoColor,
            rgb: "0,0,0",
        },
        {
            name: "Chrome",
            colorImage: ChromeColor,
            noColorImage: ChromeNoColor,
            rgb: "0,0,0",
        },
        {
            name: "Maya",
            colorImage: MayaColor,
            noColorImage: MayaNoColor,
            rgb: "0,0,0",
        },
    ]

    const tops =
        width > ScreenSize.SMALL
            ? appHighlight
                ? [110, 20, 40, 170, 400, 390]
                : [65, 20, 40, 190, 290, 370]
            : [30, 20, 110, 110, 200, 210]
    const lefts =
        width > ScreenSize.SMALL
            ? appHighlight
                ? [-50, 140, 340, 175, 125, 360]
                : [-100, 130, 340, 200, 20, 360]
            : [0, 130, 200, 70, 40, 150]

    const handleTextChange = (idx: any) => {
        setCurrentIndex(idx)
    }

    const renderBackground = () => {
        if (!appImages[currentIndex]) {
            return {
                background: "none",
                className: "website-background",
            }
        }

        const currentApp = appImages[currentIndex].name

        if (
            ["Blender"].includes(currentApp) ||
            ["Blender"].includes(appHighlight)
        ) {
            return {
                background: `linear-gradient(to bottom, rgba(255,255,255,0.8), rgba(255,255,255,0.8)), url(${BlenderTiger})`,
                className: "website-background",
            }
        } else if (
            ["Figma"].includes(currentApp) ||
            ["Figma"].includes(appHighlight)
        ) {
            return {
                background: `linear-gradient(to bottom, rgba(255,255,255,0.85), rgba(255,255,255,0.85)), url(${FigmaScreenshot})`,
                className: "website-background",
            }
        } else if (
            ["Maya"].includes(currentApp) ||
            ["Maya"].includes(appHighlight)
        ) {
            return {
                background: `linear-gradient(to bottom, rgba(255,255,255,0.5), rgba(255,255,255,0.5)), url(${MayaScreenshot})`,
                className: "website-background",
            }
        } else if (
            ["Unity"].includes(currentApp) ||
            ["Unity"].includes(appHighlight)
        ) {
            return {
                background: `linear-gradient(to bottom, rgba(255,255,255,0.85), rgba(255,255,255,0.85)), url(${UnityScreenshot})`,
                className: "website-background",
            }
        } else {
            return {
                background: `linear-gradient(to bottom, rgba(255,255,255,0.85), rgba(255,255,255,0.85)), url(${YoutubeScreenshot})`,
                className: "website-background",
            }
        }
    }

    const renderLogos = () => {
        let idx = -1
        if (appHighlight) {
            const appHighlightIndex = apps.indexOf(appHighlight + ",")
            return appImages
                .map((app: any, i: any) => {
                    if (app.name !== appHighlight) {
                        idx += idx === 2 ? 2 : 1
                        return (
                            <FloatingLogo
                                textIndex={idx}
                                boxShadow={
                                    "0px 4px 20px rgba(" + app.rgb + ", 0.2)"
                                }
                                key={"floating-icon-highlight-" + idx}
                                top={tops[idx]}
                                left={lefts[idx]}
                                colorImage={app.colorImage}
                                background="white"
                                noColorImage={app.noColorImage}
                                animationDelay={0.5 * idx + "s"}
                                app={app.name}
                            />
                        )
                    } else {
                        return []
                    }
                })
                .concat(
                    <FloatingLogo
                        textIndex={appHighlightIndex}
                        currentIndex={appHighlightIndex}
                        boxShadow={
                            "0px 4px 20px rgba(" +
                            appImages[appHighlightIndex].rgb +
                            ", 0.2)"
                        }
                        key={"floating-icon-highlight-higlighted"}
                        top={tops[3]}
                        left={lefts[3]}
                        colorImage={appImages[appHighlightIndex].colorImage}
                        noColorImage={appImages[appHighlightIndex].noColorImage}
                        background={"white"}
                        animationDelay="1.5s"
                        app={appHighlight}
                    />
                )
        } else {
            return appImages.map((app: any, _: any) => {
                idx += 1
                return (
                    <FloatingLogo
                        textIndex={idx}
                        currentIndex={currentIndex}
                        boxShadow={"0px 4px 20px rgba(" + app.rgb + ", 0.2)"}
                        key={"floating-icon-nohiglight" + idx}
                        top={tops[idx]}
                        left={lefts[idx]}
                        colorImage={app.colorImage}
                        noColorImage={app.noColorImage}
                        background={"white"}
                        animationDelay={0.5 * idx + "s"}
                        app={app.name}
                    />
                )
            })
        }
    }

    const renderedBackground = renderBackground()
    return (
        <div
            className="banner-background"
            style={{
                width: "100%",
                position: "relative",
                paddingBottom: width > ScreenSize.SMALL ? 0 : 70,
            }}
        >
            <Header dark={false} />
            <Row
                style={{
                    paddingTop: width > ScreenSize.SMALL ? 75 : 0,
                    backgroundImage:
                        width > ScreenSize.MEDIUM
                            ? renderedBackground.background
                            : "none",
                }}
                className={
                    width > ScreenSize.MEDIUM
                        ? renderedBackground.className
                        : ""
                }
            >
                <Col
                    sm={12}
                    md={7}
                    style={{
                        color: "#111111",
                        textAlign: "left",
                        paddingLeft: 0,
                        position: "relative",
                        paddingRight: 0,
                        zIndex: 1,
                        flex: width > ScreenSize.MEDIUM ? "" : "0 1 auto",
                        maxWidth: width > ScreenSize.MEDIUM ? "" : "100%",
                    }}
                >
                    <div
                        className="main-title"
                        style={{
                            marginTop: width > ScreenSize.SMALL ? 10 : 7,
                        }}
                    >
                        {appHighlight ? (
                            appHighlight + ","
                        ) : (
                            <TypeWriterEffect
                                textStyle={{ display: "inline-block" }}
                                onTextChange={handleTextChange}
                                startDelay={0}
                                cursorColor="#111111"
                                multiText={apps}
                                loop={true}
                                typeSpeed={200}
                            />
                        )}
                    </div>
                    <div
                        className="main-title"
                        style={{
                            display: "inline-block",
                            paddingBottom: 20,
                            lineHeight: 1,
                            marginBottom: 40,
                            position: "relative",
                            top: width > ScreenSize.SMALL ? 0 : "-5px",
                        }}
                    >
                        just faster.
                    </div>
                    <p className="subtitle">
                        Fractal supercharges your applications by streaming them
                        from the cloud. Join our waitlist before the countdown
                        ends for access.
                    </p>
                    <div style={{ marginTop: 50 }}>
                        <WaitlistForm dark={false} />
                    </div>
                </Col>
                <Col sm={12} md={5} className="logos-container">
                    {renderLogos()}
                </Col>
            </Row>
        </div>
    )
}

export default TopView
