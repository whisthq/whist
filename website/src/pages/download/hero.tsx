import React from "react"
import { FaApple, FaWindows } from "react-icons/fa"
import { Link } from "react-scroll"

import {
    FractalButton,
    FractalButtonState,
} from "@app/shared/components/button"
import { ScreenSize } from "@app/shared/constants/screenSizes"
import { withContext } from "@app/shared/utils/context"
import { config } from "@app/shared/constants/config"

const Hero = () => {
    /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
    const { width } = withContext()

    return (
        <div className="mb-24 text-center pt-36">
            <div className="px-0 md:px-12">
                <div className="text-6xl md:text-7xl tracking-wide leading-snug text-gray dark:text-gray-300 mb-4">
                    Try the <span className="text-mint">alpha release</span>
                </div>
                <div className="text-md md:text-lg text-gray tracking-wide dark:text-gray-400 max-w-xl m-auto">
                    Fractal is early, but we&lsquo;re excited to see what
                    you&lsquo;ll do with a lighter, faster browser. For optimal
                    experience, please review the{" "}
                    <Link to="requirements" spy={true} smooth={true}>
                        <span className="text-mint cursor-pointer">
                            requirements below
                        </span>
                    </Link>
                    .
                </div>
                {width > ScreenSize.SMALL ? (
                    <div className="flex justify-center">
                        <a href={config.client_download_urls.macOS} download>
                            <FractalButton
                                className="mt-12 mx-2"
                                contents={
                                    <div className="flex">
                                        <FaApple className="relative mr-3 top-0.5" />
                                        macOS
                                    </div>
                                }
                                state={FractalButtonState.DEFAULT}
                            />
                        </a>
                        <FractalButton
                            className="mt-12 mx-2"
                            contents={
                                <div className="flex">
                                    <FaWindows className="relative mr-3 top-0.5" />
                                    Coming Soon
                                </div>
                            }
                            state={FractalButtonState.DISABLED}
                        />
                    </div>
                ) : (
                    <div className="flex justify-center">
                        {" "}
                        <FractalButton
                            className="mt-12 mx-2"
                            contents={
                                <div className="flex">
                                    <FaApple className="relative mr-3 top-0.5" />
                                    Mac Only
                                </div>
                            }
                            state={FractalButtonState.DISABLED}
                        />
                    </div>
                )}
            </div>
        </div>
    )
}

export default Hero
