import React from "react"
import { Row, Col } from "react-bootstrap"
import { CheckIcon } from "@heroicons/react/outline"
import { HashLink } from "react-router-hash-link"

import { ScreenSize } from "@app/shared/constants/screenSizes"
import { withContext } from "@app/shared/utils/context"

import ChromeBackground from "@app/assets/graphics/speedTestBackground.svg"
import SpeedTest from "@app/assets/gifs/speedTest.gif"
import {
    AnimatedLineA,
    AnimatedLineB,
} from "@app/shared/components/animatedLines"

const features = [
    {
        name: "Built-in VPN",
        description:
            "When you visit websites on Fractal, you will not send the IP address of your computer but rather the IP of the datacenter that Fractal runs on.",
    },
    {
        name: "Browsing Data Privacy",
        description:
            "Whereas Chrome caches your browsing data (history, cookies, etc.) onto your computer, Fractal encrypts it and stores it in a server that only you can access.",
    },
    {
        name: "Computer Info Privacy",
        description:
            "Normal websites track you by pulling information about your computer's operating system, location, etc. This is impossible to do with Fractal.",
    },
    {
        name: "Malware Protection",
        description:
            "Unwanted downloads will be downloaded onto a remote server instead of infecting your computer.",
    },
    {
        name: "Local Network Protection",
        description:
            "All information sent over the network via Fractal is AES encrypted and protected against anyone spying on your network.",
    },
]

export const VerticalTemplate = (props: {
    visible: boolean
    title: JSX.Element
    text: JSX.Element
    image: JSX.Element
    background?: boolean
}) => {
    /*
        Template for arranging text and images vertically

        Arguments:
            visible (boolean): Should the image be visible
            title (Element): Title
            text (Element): Text under title
            image (Element): Image under text
            background (boolean): Should the animated background be visible
    */

    const { title, text, image, visible } = props

    return (
        <div className="mt-24 md:mt-52">
            <Row>
                <Col md={12} className="text-center">
                    <div className="text-gray dark:text-gray-300 text-4xl md:text-6xl mb-8 leading-relaxed">
                        {title}
                    </div>
                    <div className="max-w-screen-sm m-auto text-gray dark:text-gray-400 md:text-lg tracking-wider">
                        {text}
                    </div>
                </Col>
                <Col md={12} className="text-center mt-4">
                    {visible && image}
                </Col>
            </Row>
        </div>
    )
}

export const Middle = () => {
    /*
        Features section of product page

        Arguments: none
    */
    const { width } = withContext()

    return (
        <div>
            <VerticalTemplate
                visible={true}
                background
                title={
                    <>
                        <div className="text-gray dark:text-gray-300">
                            {" "}
                            Load pages{" "}
                            <span className="text-blue dark:text-mint">
                                instantly
                            </span>
                        </div>
                    </>
                }
                text={
                    <div className="text-md text-gray-400">
                        Fractal runs in the cloud on gigabit Internet. Browse
                        the web at lightning speeds, even if your own Internet
                        is slow.
                    </div>
                }
                image={
                    <div
                        className="mt-8 inline-block rounded p-2"
                        style={{
                            background:
                                "linear-gradient(233.28deg, #90ACF3 0%, #F0A8F1 100%)",
                        }}
                    >
                        <img
                            className="relative top-0 left-0 w-full max-w-screen-sm m-auto shadow-xl"
                            src={ChromeBackground}
                            alt=""
                        />
                        <img
                            className="absolute inline-block top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/3 md:-translate-y-28 w-1/2 max-w-xs"
                            src={SpeedTest}
                            alt=""
                        />
                    </div>
                }
            />
            <VerticalTemplate
                visible={true}
                title={
                    <>
                        <div className="text-gray-dark dark:text-gray-300">
                            Use <span className="text-mint">10x less</span>{" "}
                            memory
                        </div>
                    </>
                }
                text={
                    <div className="text-md text-gray-400">
                        Today, your computer runs out of memory and slows down
                        when you open too many tabs. By running in the cloud,
                        Fractal never consumes more than 300MB of memory.
                    </div>
                }
                image={
                    <div className="m-auto inline-block max-w-xs md:max-w-none">
                        <div className="relative right-32 md:right-0">
                            <AnimatedLineB
                                scale={width > ScreenSize.MEDIUM ? 1.0 : 0.5}
                            />
                        </div>
                        <div className="relative right-32 bottom-24 md:right-0 md:bottom-0">
                            <AnimatedLineA
                                scale={width > ScreenSize.MEDIUM ? 1.0 : 0.5}
                            />
                        </div>
                        <div
                            style={{ background: "rgba(255,255,255,0.1)" }}
                            className="font-body absolute px-10 py-3 text-gray-300 font-bold rounded top-4 md:top-24 right-4 md:right-64 text-xs w-56 shadow-xl"
                        >
                            <div>Chrome uses 4GB RAM</div>
                        </div>
                        <div
                            style={{ background: "rgba(255,255,255,0.1)" }}
                            className="font-body absolute px-10 py-3 text-gray-300 font-bold rounded top-56 md:top-52 right-4 md:right-64 text-xs w-56 shadow-xl"
                        >
                            <div>Fractal uses 0.3GB RAM</div>
                        </div>
                        <div className="font-body text-sm font-bold text-gray dark:text-gray-300 tracking-wide mt-0 md:pt-8 w-full">
                            RAM usage vs. number of tabs open
                        </div>
                    </div>
                }
            />
            <VerticalTemplate
                visible={true}
                background
                title={
                    <>
                        <div className="text-gray dark:text-gray-300">
                            Always{" "}
                            <span className="text-blue dark:text-mint">
                                incognito
                            </span>
                        </div>
                    </>
                }
                text={
                    <div className="mt-2 text-md text-gray-400 mb-4">
                        Because Fractal runs in a remote datacenter, your
                        browser and all its data are entirely decoupled from
                        your personal computer.
                    </div>
                }
                image={
                    <>
                        <dl className="mt-8 space-y-10 sm:space-y-0 sm:grid sm:grid-cols-2 sm:gap-x-6 sm:gap-y-12 lg:grid-cols-3 lg:gap-x-8 bg-gray-900 p-12">
                            {features.map((feature) => (
                                <div key={feature.name} className="relative">
                                    <dt>
                                        <CheckIcon
                                            className="absolute h-6 w-6 text-green-500"
                                            aria-hidden="true"
                                        />
                                        <p className="ml-9 text-lg leading-6 font-medium text-gray-300 text-left">
                                            {feature.name}
                                        </p>
                                    </dt>
                                    <dd className="mt-2 ml-9 text-base text-gray-500 text-left">
                                        {feature.description}
                                    </dd>
                                </div>
                            ))}
                        </dl>
                        <HashLink to="/security#top">
                            <button className="mt-12 text-mint py-2 px-8">
                                Read more about security
                            </button>
                        </HashLink>
                    </>
                }
            />
        </div>
    )
}

export default Middle
