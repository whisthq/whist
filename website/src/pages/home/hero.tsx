import React from "react"
import FadeIn from "react-fade-in"
import { TypeWriter } from "@app/shared/components/typewriter"
import { ChevronRightIcon } from "@heroicons/react/solid"

import { withContext } from "@app/shared/utils/context"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import YoutubeLogo from "@app/assets/icons/youtubeLogo.svg"
import NotionLogo from "@app/assets/icons/notionLogo.svg"
import ShopifyLogo from "@app/assets/icons/shopifyLogo.svg"
import AirtableLogo from "@app/assets/icons/airtableLogo.svg"
import FigmaLogo from "@app/assets/icons/figmaLogo.svg"
import GoogleDriveLogo from "@app/assets/icons/googleDriveLogo.svg"

import {
    FractalButton,
    FractalButtonState,
} from "@app/shared/components/button"

export const Hero = () => {
    /*
        Top section of Chrome product page

        Arguments: none
    */
    const { width } = withContext()
    const hovering = false

    const adjectives = ["faster", "lighter", "private"]

    return (
        <div>
            <div className="text-white text-lg m-auto text-center py-8"></div>
            <div className="text-center">
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1500 : 0}>
                    <div>
                        <a
                            href="/contact#careers"
                            className="inline-flex items-center text-gray-300 bg-gray-900 rounded-full p-1 pr-2 sm:text-base lg:text-sm xl:text-base hover:text-gray-200"
                        >
                            <span className="font-semibold px-4 py-1.5 text-gray-100 text-xs leading-5 tracking-wide bg-blue rounded-full">
                                We&lsquo;re hiring
                            </span>
                            <span className="ml-4 text-sm">Browse careers</span>
                            <ChevronRightIcon
                                className="ml-2 w-5 h-5 text-gray-500"
                                aria-hidden="true"
                            />
                        </a>
                        <div className="text-5xl md:text-8xl dark:text-gray-300 mt-16">
                            <div>
                                Chrome,
                                <div className="flex justify-center relative">
                                    <div className="mr-3 py-2">just</div>
                                    <TypeWriter
                                        words={adjectives}
                                        startAt={5}
                                        classNameCursor="bg-gray-300"
                                        classNameText="py-2 text-mint relative"
                                    />
                                </div>
                            </div>
                        </div>
                    </div>
                </FadeIn>
                <FadeIn delay={width > ScreenSize.MEDIUM ? 1700 : 100}>
                    <div className="mt-6 md:mt-8relative">
                        <p className="text-md m-auto max-w-screen-sm dark:text-gray-400 tracking-wider">
                            Load pages instantly. Use 10x less memory. Enjoy
                            complete privacy. Fractal is a supercharged version
                            of Chrome that runs in the cloud.
                        </p>
                        <a href="/download#top">
                            <FractalButton
                                className="mt-12"
                                contents="Download Now"
                                state={FractalButtonState.DEFAULT}
                            />
                        </a>
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
                            src={NotionLogo}
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
                            src={ShopifyLogo}
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
                            src={GoogleDriveLogo}
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
                            src={AirtableLogo}
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

export default Hero
