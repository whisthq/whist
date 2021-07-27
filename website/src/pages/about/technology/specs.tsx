import React from "react"

import {
    FractalButton,
    FractalButtonState,
} from "@app/shared/components/button"

const Specs = () => {
    return (
        <div className="max-w-7xl mx-auto mt-24 py-12 px-4 sm:py-16 lg:py-20">
            <div className="max-w-4xl mx-auto text-center">
                <div className="text-3xl text-gray-300 sm:text-4xl">
                    Chrome, but more powerful
                </div>
                <p className="mt-3 text-xl text-gray-500 sm:mt-4">
                    Fractal runs Google Chrome on datacenter-grade hardware
                </p>
            </div>
            <dl className="mt-10 text-center sm:max-w-3xl sm:mx-auto sm:grid sm:grid-cols-3 sm:gap-8">
                <div className="flex flex-col">
                    <dt className="order-2 mt-2 text-lg leading-6 font-medium text-blue">
                        NVIDIA T4 GPU
                    </dt>
                    <dd className="order-1 text-5xl font-extrabold text-white">
                        1
                    </dd>
                </div>
                <div className="flex flex-col mt-10 sm:mt-0">
                    <dt className="order-2 mt-2 text-lg leading-6 font-medium text-blue">
                        CPU Cores
                    </dt>
                    <dd className="order-1 text-5xl font-extrabold text-white">
                        6
                    </dd>
                </div>
                <div className="flex flex-col mt-10 sm:mt-0">
                    <dt className="order-2 mt-2 text-lg leading-6 font-medium text-blue">
                        GB RAM
                    </dt>
                    <dd className="order-1 text-5xl font-extrabold text-white">
                        8
                    </dd>
                </div>
            </dl>
            <div className="text-center mt-16">
                <FractalButton
                    contents="DOWNLOAD NOW"
                    state={FractalButtonState.DEFAULT}
                />
            </div>
        </div>
    )
}

export default Specs
