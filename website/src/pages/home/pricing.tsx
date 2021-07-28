import React from "react"
import { CheckIcon } from "@heroicons/react/solid"
import { HashLink } from "react-router-hash-link"

import {
    FractalButton,
    FractalButtonState,
} from "@app/shared/components/button"

const includedFeatures = [
    "Unlimited access until Jan 1, 2022",
    "Private Discord forum access",
    "Recognition on supporter board (coming soon)",
    "Priority customer support",
]

const Pricing = () => {
    return (
        <div className="mt-24">
            <div className="pt-12 sm:pt-16 lg:pt-20">
                <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
                    <div className="text-center">
                        <div className="text-3xl text-gray-300 sm:text-4xl lg:text-5xl mb-4">
                            Try Fractal for free
                        </div>
                        <p className="m-auto text-lg text-gray-500 max-w-screen-sm">
                            You&lsquo;ll start with a 14 day free trial, after
                            which you&lsquo;ll have to option to continue using
                            Fractal by becoming a supporter.
                        </p>
                        <HashLink to="/download#top">
                            <FractalButton
                                className="mt-8 mb-4"
                                contents="DOWNLOAD NOW"
                                state={FractalButtonState.DEFAULT}
                            />
                        </HashLink>
                    </div>
                </div>
            </div>
            <div className="mt-8 sm:mt-12">
                <div className="relative">
                    <div className="relative">
                        <div className="max-w-lg mx-auto overflow-hidden lg:max-w-none lg:flex lg:items-start">
                            <div className="bg-gray-900 px-6 py-8 lg:p-12 rounded-lg">
                                <div className="flex justify-between w-full">
                                    <h3 className="text-2xl text-gray-300 sm:text-3xl">
                                        Become a Supporter
                                    </h3>
                                    <div className="flex items-center justify-center text-3xl text-gray-200">
                                        <span>$30</span>
                                        <span className="ml-2 text-sm text-gray-400">
                                            USD
                                        </span>
                                    </div>
                                </div>
                                <p className="mt-6 text-base text-gray-400 tracking-wider">
                                    You can continue using Fractal after your
                                    14-day free trial by becoming a supporter
                                    via a one-time, $30 supporter fee. This is
                                    because our current focus is building a
                                    great product, not revenue. This offer
                                    expires on Jan 1, 2022, at which point
                                    we&lsquo;ll ask our supporters to help us
                                    decide a fair, long-term pricing model.
                                </p>
                                <div className="mt-8">
                                    <div className="flex items-center">
                                        <h4 className="flex-shrink-0 pr-4 text-sm tracking-wider font-semibold uppercase text-mint">
                                            Supporter Perks
                                        </h4>
                                        <div className="flex-1 border-t-2 border-gray-700" />
                                    </div>
                                    <ul className="mt-8 space-y-5 lg:space-y-0 lg:grid lg:grid-cols-2 lg:gap-x-8 lg:gap-y-5">
                                        {includedFeatures.map((feature) => (
                                            <li
                                                key={feature}
                                                className="flex items-start lg:col-span-1"
                                            >
                                                <div className="flex-shrink-0">
                                                    <CheckIcon
                                                        className="h-5 w-5 text-green-400"
                                                        aria-hidden="true"
                                                    />
                                                </div>
                                                <p className="ml-3 text-sm text-gray-400 top-0.5 relative">
                                                    {feature}
                                                </p>
                                            </li>
                                        ))}
                                    </ul>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

export default Pricing
