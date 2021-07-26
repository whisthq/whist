import React from "react"

import QuestionMan from "@app/assets/largeGraphics/questionMan.svg"

const Support = () => {
    return (
        <div className="relative bg-gray-800 py-16 sm:py-24">
            <div className="lg:mx-auto lg:max-w-7xl lg:px-8 lg:grid lg:grid-cols-2 lg:gap-24 lg:items-start">
                <div className="relative sm:py-16 lg:py-0">
                    <div className="relative mx-auto max-w-md px-4 sm:max-w-3xl sm:px-6 lg:px-0 lg:max-w-none">
                        <div className="relative pt-72 pb-16 overflow-hidden">
                            <img
                                className="absolute inset-0 h-full w-full"
                                src={QuestionMan}
                                alt=""
                            />
                        </div>
                    </div>
                </div>

                <div className="relative mx-auto max-w-md px-4 sm:max-w-3xl sm:px-6 lg:px-0">
                    {/* Content area */}
                    <div className="pt-12 sm:pt-16 lg:pt-20">
                        <div className="text-3xl text-gray-300 sm:text-4xl">
                            Don&lsquo;t see your question?
                        </div>
                        <div className="mt-6 text-gray-500 space-y-6">
                            <p className="text-lg">
                                You can join our Discord to chat with engineers
                                and other users or contact support.
                            </p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

export default Support
