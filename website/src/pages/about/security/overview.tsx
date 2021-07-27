import React from "react"

import {
    DatabaseIcon,
    BookOpenIcon,
    PlayIcon,
    KeyIcon,
} from "@heroicons/react/outline"

const features = [
    {
        name: "Browsing History",
        description:
            "Your browsing history is locked behind a master key that only you have access to.",
        icon: BookOpenIcon,
    },
    {
        name: "Passwords and Cookies",
        description:
            "All your passwords and cookies are encrypted and never touch our permanent storage.",
        icon: KeyIcon,
    },
    {
        name: "Video and Audio",
        description:
            "Your video and audio packets are encrypted along every step of the streaming process and are never tracked, stored, or seen by anyone besides you.",
        icon: PlayIcon,
    },
    {
        name: "Account Data",
        description:
            "We will never sell or release any of your data to any third-party provider without your explicit approval.",
        icon: DatabaseIcon,
    },
]

const Overview = () => {
    return (
        <div className="py-16 mt-36 lg:mt-12 bg-gray-800">
            <div className="max-w-7xl mx-auto px-8">
                <div className="lg:text-center">
                    <p className="mt-2 text-2xl leading-8 text-gray-300 lg:text-4xl">
                        What Fractal Keeps Private
                    </p>
                    <p className="mt-2 max-w-2xl text-lg lg:text-xl text-gray-500 lg:mx-auto">
                        Fractal ensures that nobody, not even our engineers, can
                        view your browsing information. The following things are
                        visible only to you:
                    </p>
                </div>

                <div className="mt-16">
                    <dl className="space-y-10 md:space-y-0 md:grid md:grid-cols-2 md:gap-x-8 md:gap-y-10">
                        {features.map((feature) => (
                            <div key={feature.name} className="relative">
                                <dt>
                                    <div className="absolute flex items-center justify-center h-12 w-12 rounded-md bg-indigo-500 text-white">
                                        <feature.icon
                                            className="h-6 w-6"
                                            aria-hidden="true"
                                        />
                                    </div>
                                    <p className="ml-20 text-lg leading-6 font-medium text-gray-300">
                                        {feature.name}
                                    </p>
                                </dt>
                                <dd className="mt-2 ml-20 text-base text-gray-500">
                                    {feature.description}
                                </dd>
                            </div>
                        ))}
                    </dl>
                </div>
            </div>
        </div>
    )
}

export default Overview
