import React from "react"
import {
    GlobeIcon,
    WifiIcon,
    DesktopComputerIcon,
} from "@heroicons/react/outline"

const features = [
    {
        name: "North America or Europe",
        description:
            "Our servers are located in various parts of the United States, southern Canada, and Europe. If you are not in these areas, you will experience high network latency.",
        icon: GlobeIcon,
    },
    {
        name: "Stable 100Mbps+ Internet",
        description: (
            <div>
                We recommend a 100Mbps+ Internet connection as a proxy for
                network stability. To accurately test if your Internet is stable
                enough, visit{" "}
                <a
                    href="https://www.meter.net/ping-test/"
                    target="_blank"
                    className="text-gray-300 hover:text-mint"
                    rel="noreferrer"
                >
                    this website
                </a>
                . A stable Internet connection has consistent sub-75ms ping with
                sub-10ms jitter.
            </div>
        ),
        icon: WifiIcon,
    },
    {
        name: "Post-2016 Mac",
        description:
            "Fractal uses your Mac's hardware decoding chip, which exists on all Macs produced during or after 2016.",
        icon: DesktopComputerIcon,
    },
]

const Requirements = () => {
    return (
        <div className="overflow-hidden mt-20" id="requirements">
            <div className="relative max-w-7xl mx-auto py-6 px-4 sm:px-6 lg:px-8">
                <div className="relative lg:grid lg:grid-cols-3 lg:gap-x-8">
                    <div className="lg:col-span-1">
                        <div className="text-3xl text-gray-300 sm:text-4xl pr-8">
                            Fractal download requirements
                        </div>
                    </div>
                    <dl className="mt-10 space-y-10 sm:space-y-0 sm:grid sm:grid-cols-2 sm:gap-x-8 sm:gap-y-10 lg:mt-0 lg:col-span-2">
                        {features.map((feature) => (
                            <div key={feature.name}>
                                <dt>
                                    <div className="flex items-center justify-center h-12 w-12 rounded-md bg-blue text-gray-300">
                                        <feature.icon
                                            className="h-6 w-6"
                                            aria-hidden="true"
                                        />
                                    </div>
                                    <p className="mt-5 text-lg leading-6 font-medium text-gray-300">
                                        {feature.name}
                                    </p>
                                </dt>
                                <dd className="mt-2 text-base text-gray-500">
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

export default Requirements
