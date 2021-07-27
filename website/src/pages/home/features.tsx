import React from "react"
import { HashLink } from "react-router-hash-link"
import {
    LightningBoltIcon,
    ChipIcon,
    ShieldCheckIcon,
} from "@heroicons/react/outline"

const supportLinks = [
    {
        name: "Lightning Speeds",
        description:
            "Fractal’s proprietary streaming technology runs at 60+ frames per second with no noticeable lag.",
        icon: LightningBoltIcon,
    },
    {
        name: "Dedicated GPU",
        description:
            "Fractal runs Chrome on a dedicated NVIDIA graphics card for the fastest page rendering speeds.",
        icon: ChipIcon,
    },
    {
        name: "Secure Environment",
        description:
            "Fractal never tracks your sessions. All information sent over the Internet, including your browsing session, is end-to-end encrypted.",
        icon: ShieldCheckIcon,
    },
]

export const Features = () => {
    /*
        Features section of Chrome product page

        Arguments:
            none
    */

    return (
        <div className="mt-32 text-center">
            <div className="pt-24">
                <div className="text-3xl md:text-5xl dark:text-gray-300 leading-normal">
                    The first{" "}
                    <span className="text-blue dark:text-mint">
                        cloud-powered
                    </span>{" "}
                    browser
                </div>
            </div>
            <div className="grid grid-cols-1 gap-y-20 lg:grid-cols-3 lg:gap-y-0 lg:gap-x-8 mt-12 text-left">
                {supportLinks.map((link) => (
                    <div
                        key={link.name}
                        className="flex flex-col rounded bg-gray-900"
                    >
                        <div className="flex-1 relative pt-16 px-6 pb-8 md:px-8">
                            <div className="top-0 p-3 inline-block bg-blue rounded transform -translate-y-1/2">
                                <link.icon
                                    className="h-5 w-5 text-white"
                                    aria-hidden="true"
                                />
                            </div>
                            <h3 className="text-xl font-medium text-gray-300">
                                {link.name}
                            </h3>
                            <p className="mt-4 text-base text-gray-500">
                                {link.description}
                            </p>
                        </div>
                    </div>
                ))}
            </div>
            <HashLink to="/technology#top">
                <button className="mt-12 text-mint py-2 px-8 rounded-3xl mb-32 hover:text-mint">
                    Read more about the technology
                </button>
            </HashLink>
        </div>
    )
}

export default Features
