import React from "react"
import { Link } from "react-scroll"

import LaptopMan from "@app/assets/graphics/laptopMan.svg"
import {
    FractalButton,
    FractalButtonState,
} from "@app/shared/components/button"

const Hero = () => (
    <div className="pt-10 sm:pt-16 lg:pt-8 lg:pb-14 lg:overflow-hidden">
        <div className="lg:grid lg:grid-cols-2 lg:gap-8">
            <div className="mx-auto max-w-md px-4 sm:max-w-2xl sm:px-6 sm:text-center lg:px-0 lg:text-left lg:flex lg:items-center">
                <div className="lg:py-24">
                    <h1 className="mt-4 text-4xl text-gray-300 sm:mt-5 sm:text-6xl lg:mt-6 xl:text-6xl">
                        We are obsessed with{" "}
                        <span className="text-mint">speed</span> and{" "}
                        <span className="text-mint">performance</span>
                    </h1>
                    <p className="mt-3 text-base text-gray-500 sm:mt-5 sm:text-xl lg:text-lg xl:text-xl">
                        Fractal develops on the cutting edge of streaming and
                        app virtualization technology
                    </p>
                    <Link to="details" spy={true} smooth={true}>
                        <FractalButton
                            className="mt-8 mb-4"
                            contents="HOW IT WORKS"
                            state={FractalButtonState.DEFAULT}
                        />
                    </Link>
                </div>
            </div>
            <div className="mt-12 -mb-16 sm:-mb-48 lg:m-0 lg:relative">
                <div className="mx-auto max-w-md px-4 sm:max-w-2xl sm:px-6 lg:max-w-none lg:px-0">
                    {/* Illustration taken from Lucid Illustrations: https://lucid.pixsellz.io/ */}
                    <img
                        className="w-full lg:absolute lg:inset-y-0 lg:left-0 lg:h-full lg:w-auto lg:max-w-none"
                        src={LaptopMan}
                        alt=""
                    />
                </div>
            </div>
        </div>
    </div>
)

export default Hero
