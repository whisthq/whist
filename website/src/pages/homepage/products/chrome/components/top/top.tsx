import React, { useState, useContext } from "react"
import { Link } from "react-router-dom"
import FadeIn from "react-fade-in"
import TypeWriterEffect from "react-typewriter-effect"

import MainContext from "shared/context/mainContext"
import { ScreenSize } from "shared/constants/screenSizes"
import Geometric from "shared/components/geometric/geometric"
import YoutubeLogo from "assets/icons/youtubeLogo.svg"
import InstagramLogo from "assets/icons/instagramLogo.svg"
import FacebookLogo from "assets/icons/facebookLogo.svg"
import RedditLogo from "assets/icons/redditLogo.svg"
import FigmaLogo from "assets/icons/figmaLogo.svg"
import GmailLogo from "assets/icons/gmailLogo.svg"

import sharedStyles from "styles/shared.module.css"

export const Top = () => {
    /*
        Top section of Chrome product page
 
        Arguments: none
    */
    const { width } = useContext(MainContext)
    const [hovering, setHovering] = useState(false)

    const adjectives = ["faster", "lighter", "private"]

    return (
        <div>
            {width > ScreenSize.MEDIUM && (
                <>
                    <FadeIn>
                        <div className="relative pointer-events-none">
                            <div
                                className="absolute"
                                style={{
                                    top: 400,
                                    left: 425,
                                }}
                            >
                                <Geometric scale={3} flip={false} />
                            </div>
                        </div>
                    </FadeIn>
                    <FadeIn>
                        <div className="relative pointer-events-none">
                            <div
                                className="absolute"
                                style={{
                                    top: 400,
                                    right: -840,
                                }}
                            >
                                <Geometric scale={3} flip={true} />
                            </div>
                        </div>
                    </FadeIn>
                </>
            )}
            <div className="mt-16 text-center">
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1500 : 0}>
                    <div className="text-5xl md:text-8xl dark:text-gray-300">
                        <div>
                            Chrome,
                            <div className="flex justify-center relative top-2">
                                <div className="mr-3">just</div>
                                <div className="text-blue dark:text-mint relative bottom-1 md:bottom-2.5">
                                    <TypeWriterEffect
                                        textStyle={{
                                            display: "inline-block",
                                            fontWeight: "normal",
                                            fontSize:
                                                width > ScreenSize.MEDIUM
                                                    ? 100
                                                    : 50,
                                            marginLeft:
                                                width > ScreenSize.MEDIUM
                                                    ? 12
                                                    : 0,
                                        }}
                                        // onTextChange={handleTextChange}
                                        startDelay={0}
                                        cursorColor="#553DE9"
                                        multiText={adjectives}
                                        loop={true}
                                        typeSpeed={200}
                                    />
                                </div>
                            </div>
                        </div>
                    </div>
                </FadeIn>
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1700 : 100}>
                    <div className="mt-10 md:mt-12 relative">
                        <p className="m-auto max-w-screen-sm dark:text-gray-400 tracking-wider">
                            Load pages instantly. Use 10x less memory. Enjoy
                            complete privacy. Fractal is a supercharged version
                            of Chrome that runs in the cloud.
                        </p>
                        <Link to="/auth">
                            <button
                                className="rounded bg-blue dark:bg-mint dark:text-black px-8 py-3 mt-8 text-white w-full md:w-48 hover:bg-mint hover:text-black duration-500 mt-12 tracking-wide"
                                style={{ opacity: 1.0 }}
                                onMouseEnter={() => setHovering(true)}
                                onMouseLeave={() => setHovering(false)}
                            >
                                <div>GET STARTED</div>
                            </button>
                        </Link>
                    </div>
                </FadeIn>
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1900 : 200}>
                    <div className="relative max-w-screen-sm h-80 m-auto text-center">
                        <img
                            src={YoutubeLogo}
                            style={{
                                position: "absolute",
                                left: width > ScreenSize.MEDIUM ? 45 : 25,
                                top: width > ScreenSize.MEDIUM ? 170 : 125,
                                width: width > ScreenSize.MEDIUM ? 70 : 50,
                                opacity: hovering ? 0.15 : 0.25,
                            }}
                            className={sharedStyles.bounce}
                            alt="youtube"
                        />
                        <img
                            src={FigmaLogo}
                            style={{
                                position: "absolute",
                                left: width > ScreenSize.MEDIUM ? 250 : 150,
                                top: width > ScreenSize.MEDIUM ? 80 : 50,
                                width: width > ScreenSize.MEDIUM ? 40 : 30,
                                opacity: hovering ? 0.15 : 0.25,
                                animationDelay: "0.9s",
                            }}
                            className={sharedStyles.bounce}
                            alt="figma"
                        />
                        <img
                            src={InstagramLogo}
                            style={{
                                position: "absolute",
                                left: width > ScreenSize.MEDIUM ? 170 : 150,
                                top: width > ScreenSize.MEDIUM ? 280 : 175,
                                width: width > ScreenSize.MEDIUM ? 45 : 35,
                                opacity: hovering ? 0.15 : 0.25,
                                animationDelay: "0.5s",
                            }}
                            className={sharedStyles.bounce}
                            alt="instagram"
                        />
                        <img
                            src={FacebookLogo}
                            style={{
                                position: "absolute",
                                left: width > ScreenSize.MEDIUM ? 370 : 220,
                                width: width > ScreenSize.MEDIUM ? 50 : 35,
                                top: width > ScreenSize.MEDIUM ? 210 : 110,
                                opacity: hovering ? 0.15 : 0.25,
                                animationDelay: "0.2s",
                            }}
                            className={sharedStyles.bounce}
                            alt="facebook"
                        />
                        <img
                            src={GmailLogo}
                            style={{
                                position: "absolute",
                                left: width > ScreenSize.MEDIUM ? 450 : 30,
                                top: width > ScreenSize.MEDIUM ? 60 : 210,
                                width: width > ScreenSize.MEDIUM ? 50 : 35,
                                opacity: hovering ? 0.15 : 0.25,
                                animationDelay: "0.7s",
                            }}
                            className={sharedStyles.bounce}
                            alt="gmail"
                        />
                        <img
                            src={RedditLogo}
                            style={{
                                position: "absolute",
                                left: width > ScreenSize.MEDIUM ? 500 : 240,
                                top: width > ScreenSize.MEDIUM ? 350 : 240,
                                width: width > ScreenSize.MEDIUM ? 50 : 35,
                                opacity: hovering ? 0.15 : 0.2,
                                animationDelay: "0.1s",
                            }}
                            className={sharedStyles.bounce}
                            alt="reddit"
                        />
                    </div>
                </FadeIn>
            </div>
        </div>
    )
}

export default Top
