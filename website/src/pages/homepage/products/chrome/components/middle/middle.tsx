import React, { useState } from "react"

import ChromeBackground from "assets/largeGraphics/speedTestBackground.svg"
import SpeedTest from "assets/gifs/speedTest.gif"
import VisibilitySensor from "react-visibility-sensor"
import AnimatedLineA from "pages/homepage/products/chrome/components/middle/components/animatedLine/animatedLineA"
import AnimatedLineB from "pages/homepage/products/chrome/components/middle/components/animatedLine/animatedLineB"
import Router from "assets/icons/router.svg"
import BrowsingData from "assets/icons/browsingData.svg"

import VerticalTemplate from "pages/homepage/products/chrome/components/middle/components/verticalTemplate/verticalTemplate"

export const Middle = () => {
    /*
        Pricing section of Blender product page
 
        Arguments: none
    */
    const [visible, setVisible] = useState(false)
    // TODO: Mobile Compatibility
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
                    <div className="dark:text-gray-400 tracking-wider">
                        Fractal runs in the cloud on gigabit Internet. Browse
                        the web at lightning speeds, even if your own Internet
                        is slow.
                    </div>
                }
                image={
                    <div className="mt-8 inline-block shadow-bright">
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
            <VisibilitySensor
                onChange={(isVisible) => {
                    if (isVisible) {
                        setVisible(true)
                    }
                }}
            >
                <VerticalTemplate
                    visible={visible}
                    title={
                        <>
                            <div className="text-gray-dark dark:text-gray-300">
                                Use <span className="text-mint">10x less</span>{" "}
                                memory
                            </div>
                        </>
                    }
                    text={
                        <div className="dark:text-gray-400 tracking-wider">
                            Today, your computer runs out of memory and slows
                            down when you open too many tabs. By running in the
                            cloud, Fractal never consumes more than 300MB of
                            memory.
                        </div>
                    }
                    image={
                        <div className="m-auto inline-block max-w-xs md:max-w-none">
                            <div className="relative right-32 md:right-0">
                                <AnimatedLineB
                                    scale={1.0}
                                />
                            </div>
                            <div className="relative right-32 bottom-24 md:right-0 md:bottom-0">
                                <AnimatedLineA
                                    scale={1.0}
                                />
                            </div>
                            <div className="absolute bg-blue-light px-10 py-3 text-blue font-bold rounded top-4 md:top-24 right-4 md:right-64 text-xs w-56 shadow-xl">
                                <div>Chrome uses 4GB RAM</div>
                            </div>
                            <div className="absolute bg-mint-light px-10 py-3 text-gray font-bold rounded top-56 md:top-52 right-4 md:right-64 text-xs w-56 shadow-xl">
                                <div>Fractal uses 0.3GB RAM</div>
                            </div>
                            <div className="text-sm font-bold text-gray dark:text-gray-300 tracking-wide mt-0 md:pt-8 w-full">
                                RAM usage vs. number of tabs open
                            </div>
                        </div>
                    }
                />
            </VisibilitySensor>
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
                    <div className="dark:text-gray-400 tracking-wider mb-4">
                        Because Fractal runs in datacenters, your IP address and
                        location are hidden from websites (similar to a VPN),
                        and your browsing data is not stored on your computer.
                    </div>
                }
                image={
                    <div
                        className="rounded border-2 border-white border-solid padding px-4 md:px-10 py-4 shadow-xl w-72 md:w-96 m-auto text-white tracking-wide mt-16 text-sm shadow-bright"
                        style={{
                            background: "#0E042C",
                        }}
                    >
                        <div className="max-h-10 md:max-h-12 w-full pt-3">
                            <div className="flex w-full">
                                <div>
                                    <img
                                        src={Router}
                                        className="relative w-6 mr-4 bottom-1"
                                        alt="router"
                                    />
                                </div>
                                <div className="text-lg text-gray-300 tracking-wider">
                                    IP Address
                                </div>
                            </div>
                            <div
                                className="relative bg-blue-light text-blue px-4 md:px-8 py-2.5 rounded text-xs font-bold w-36 tracking-wide bottom-8"
                                style={{
                                    left: 275
                                }}
                            >
                                Not Traced
                            </div>
                        </div>
                        <div className="max-h-10 md:max-h-12 w-full mt-4">
                            <div className="flex w-full">
                                <div>
                                    <img
                                        src={BrowsingData}
                                        className="w-6 mr-4"
                                        alt="data"
                                    />
                                </div>
                                <div className="text-lg text-gray-300 tracking-wider">
                                    Browsing Data
                                </div>
                            </div>
                            <div
                                className="relative animate-bounce bg-mint text-gray px-4 md:px-8 py-2.5 rounded text-xs font-bold w-36 tracking-wide bottom-6 delay-500 duration-500"
                                style={{
                                    left: 275
                                }}
                            >
                                Not Stored
                            </div>
                        </div>
                    </div>
                }
            />
        </div>
    )
}

export default Middle
