import React, { useState, useContext } from "react"
import { Link } from "react-router-dom"
import FadeIn from "react-fade-in"
import { TypeWriter } from "@app/shared/components/typewriter"
import classNames from "classnames"

import MainContext from "@app/shared/context/mainContext"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import Geometric from "./geometric"
import YoutubeLogo from "@app/assets/icons/youtubeLogo.svg"
import InstagramLogo from "@app/assets/icons/instagramLogo.svg"
import FacebookLogo from "@app/assets/icons/facebookLogo.svg"
import RedditLogo from "@app/assets/icons/redditLogo.svg"
import FigmaLogo from "@app/assets/icons/figmaLogo.svg"
import GmailLogo from "@app/assets/icons/gmailLogo.svg"

import Typeform from "@app/pages/home/components/typeform"

const SymmetricGeometric = (props: any) => (
    <FadeIn>
        <div
            className={props.className}
            style={{
                top: 400,
                left: 425,
            }}
        >
            <Geometric
                className="relative pointer-events-none"
                scale={3}
                flip={false}
            />
        </div>
        <div
            className={props.className}
            style={{
                top: 400,
                right: -840,
            }}
        >
            <Geometric
                className="relative pointer-events-none"
                scale={3}
                flip={true}
            />
        </div>
    </FadeIn>
)

export const Top = () => {
    /*
        Top section of Chrome product page

        Arguments: none
    */
    const { width } = useContext(MainContext)
    const [hovering, setHovering] = useState(false)
    const [show, setShow] = useState(false)

    const adjectives = ["faster", "lighter", "private"]

    return (
        <div>
            <div className="text-white text-lg m-auto text-center py-8">
            </div>
            {width > ScreenSize.MEDIUM && <SymmetricGeometric className="absolute"/>}
            <div className="mt-16 text-center">
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1500 : 0}>
                    <div className="text-5xl md:text-8xl dark:text-gray-300">
                        <div>
                            Chrome,
                            <div className="flex justify-center relative">
                                <div className="mr-3 py-2">just</div>
                                <TypeWriter
                                    words={adjectives}
                                    startAt={5}
                                    classNameCursor="bg-blue-800"
                                    classNameText="py-2 text-blue dark:text-mint relative"
                                />
                            </div>
                        </div>
                    </div>
                </FadeIn>
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1700 : 100}>
                    <div className="mt-10 md:mt-12 relative">
                        <p className="font-body m-auto max-w-screen-sm dark:text-gray-400 tracking-wider">
                            Load pages instantly. Use 10x less memory. Enjoy
                            complete privacy. Fractal is a supercharged version
                            of Chrome that runs in the cloud.
                        </p>
                        <Typeform text="JOIN WAITLIST"/>
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
                            className="animate-bounce"
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
                            className="animate-bounce"
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
                            className="animate-bounce"
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
                            className="animate-bounce"
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
                            className="animate-bounce"
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
                            className="animate-bounce"
                            alt="reddit"
                        />
                    </div>
                </FadeIn>
            </div>
        </div>
    )
}

export default Top
