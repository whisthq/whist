import React, { useState, useContext } from "react"
import TypeWriterEffect from "react-typewriter-effect"
import { Row, Col } from "react-bootstrap"

import WaitlistForm from "shared/components/waitlistForm"
import Header from "shared/components/header"
import MainContext from "shared/context/mainContext"
import FloatingLogo from "pages/landing/components/floatingLogo"

import BlenderNoColor from "assets/largeGraphics/blenderNoColor.svg"
import VSCodeNoColor from "assets/largeGraphics/vscodeNoColor.svg"
import FigmaNoColor from "assets/largeGraphics/figmaNoColor.svg"
import ChromeNoColor from "assets/largeGraphics/chromeNoColor.svg"
import MayaNoColor from "assets/largeGraphics/mayaNoColor.svg"
import PhotoshopNoColor from "assets/largeGraphics/photoshopNoColor.svg"
import BlenderColor from "assets/largeGraphics/blenderColor.svg"
import VSCodeColor from "assets/largeGraphics/vscodeColor.svg"
import FigmaColor from "assets/largeGraphics/figmaColor.svg"
import ChromeColor from "assets/largeGraphics/chromeColor.svg"
import MayaColor from "assets/largeGraphics/mayaColor.svg"
import PhotoshopColor from "assets/largeGraphics/photoshopColor.svg"

import "styles/landing.css"

const TopView = (props: any) => {
    const { width, appHighlight } = useContext(MainContext)

    const [currentIndex, setCurrentIndex] = useState(0)

    const apps = [
        "Adobe,",
        "Blender,",
        "Figma,",
        "VSCode,",
        "Chrome,",
        "Maya,",
        "Your apps,",
    ]

    const appImages = [
        {
            name: "Adobe",
            colorImage: PhotoshopColor,
            noColorImage: PhotoshopNoColor,
            rgb: "36, 35, 85",
        },
        {
            name: "Blender",
            colorImage: BlenderColor,
            noColorImage: BlenderNoColor,
            rgb: "234, 118, 0",
        },
        {
            name: "Figma",
            colorImage: FigmaColor,
            noColorImage: FigmaNoColor,
            rgb: "162, 89, 255",
        },
        {
            name: "VSCode",
            colorImage: VSCodeColor,
            noColorImage: VSCodeNoColor,
            rgb: "0, 120, 215",
        },
        {
            name: "Chrome",
            colorImage: ChromeColor,
            noColorImage: ChromeNoColor,
            rgb: "66, 133, 244",
        },
        {
            name: "Maya",
            colorImage: MayaColor,
            noColorImage: MayaNoColor,
            rgb: "115, 194, 251",
        },
    ]

    const tops =
        width > 720
            ? appHighlight
                ? [110, 20, 40, 170, 420, 390]
                : [65, 20, 40, 190, 350, 370]
            : [30, 20, 110, 110, 200, 210]
    const lefts =
        width > 720
            ? appHighlight
                ? [-50, 140, 410, 200, 150, 450]
                : [-100, 130, 410, 250, 150, 450]
            : [0, 130, 200, 70, 40, 150]

    const handleTextChange = (idx: any) => {
        setCurrentIndex(idx)
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
                                width={width}
                                top={tops[idx]}
                                left={lefts[idx]}
                                colorImage={app.colorImage}
                                noColorImage={app.noColorImage}
                                background={"rgba(" + app.rgb + ", 0.1)"}
                                animationDelay={0.5 * idx + "s"}
                                app={app.name}
                            />
                        )
                    } else {
                        return <></>
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
                        width={width}
                        top={tops[3]}
                        left={lefts[3]}
                        colorImage={appImages[appHighlightIndex].colorImage}
                        noColorImage={appImages[appHighlightIndex].noColorImage}
                        background={
                            "rgba(" +
                            appImages[appHighlightIndex].rgb +
                            ", 0.1)"
                        }
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
                        width={width}
                        top={tops[idx]}
                        left={lefts[idx]}
                        colorImage={app.colorImage}
                        noColorImage={app.noColorImage}
                        background={"rgba(" + app.rgb + ", 0.1)"}
                        animationDelay={0.5 * idx + "s"}
                        app={app.name}
                    />
                )
            })
        }
    }

    return (
        <div
            className="banner-background"
            style={{
                width: "100%",
                position: "relative",
                zIndex: 1,
                paddingBottom: width > 720 ? 0 : 70,
            }}
        >
            <Header color="black" />
            <Row
                style={{
                    width: "100%",
                    padding: 0,
                    margin: 0,
                    paddingTop: width > 720 ? 75 : 0,
                }}
            >
                <Col
                    md={7}
                    style={{
                        color: "#111111",
                        textAlign: "left",
                        paddingLeft: 0,
                        position: "relative",
                        paddingRight: 0,
                    }}
                >
                    <div
                        style={{
                            fontFamily: "Maven Pro",
                            zIndex: 100,
                            fontSize: width > 720 ? 80 : 50,
                            fontWeight: "bold",
                            color: "black",
                            marginTop: width > 720 ? 10 : 7,
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
                        style={{
                            fontSize: width > 720 ? 80 : 50,
                            fontWeight: "bold",
                            display: "inline-block",
                            paddingBottom: 20,
                            zIndex: 100,
                            lineHeight: width > 720 ? 1.0 : 0.6,
                            marginBottom: 40,
                        }}
                    >
                        just <span style={{ color: "#111111" }}>faster</span>.
                    </div>
                    <p
                        style={{
                            lineHeight: 1.6,
                            color: "#333333",
                            letterSpacing: 1.5,
                            textAlign: "left",
                        }}
                    >
                        Fractal uses cloud streaming to supercharge your
                        laptop's laggy applications. Join our waitlist before
                        the countdown ends for access.
                    </p>
                    <div style={{ marginTop: 50, zIndex: 100 }}>
                        <WaitlistForm />
                    </div>
                </Col>
                <Col
                    md={5}
                    style={{
                        textAlign: "right",
                        paddingLeft: 100,
                        marginRight: 0,
                        paddingRight: 0,
                        paddingTop: 30,
                        position: "relative",
                        top: width > 720 ? 0 : 20,
                    }}
                >
                    {renderLogos()}
                </Col>
            </Row>
        </div>
    )
}

export default TopView
