import React from "react"
import {
    GlobeAltIcon,
    LightningBoltIcon,
    MailIcon,
    ScaleIcon,
} from "@heroicons/react/outline"

const features = [
    {
        name: "Tab Collectors",
        description:
            "When leaving several dozen tabs open consumes gigabytes of memory and tab suspender extensions don't do the job.",
        icon: GlobeAltIcon,
    },
    {
        name: "Product Designers",
        description:
            "When working with large assets in 2D/3D, web-based design tools like Figma.",
        icon: ScaleIcon,
    },
    {
        name: "Productivity Workers",
        description:
            "When having multiple Google Slides, Airtables, or Notion Docs open slows down your computer.",
        icon: LightningBoltIcon,
    },
    {
        name: "Content Consumers",
        description:
            "When you want any website to load at the speed of thought and videos to never buffer.",
        icon: MailIcon,
    },
]

const Users = () => {
    return (
        <div className="overflow-hidden mt-36 bg-blue-darker">
            <div className="relative max-w-7xl mx-auto py-24 px-4 sm:px-6 lg:px-8">
                <div className="relative lg:grid lg:grid-cols-3 lg:gap-x-8">
                    <div className="lg:col-span-1">
                        <div className="text-3xl dark:text-gray-300 sm:text-4xl">
                            For those who push Chrome to its limit.
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

export default Users