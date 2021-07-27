import React from "react"

import {
    FractalButton,
    FractalButtonState,
} from "@app/shared/components/button"

const Specs = () => {
    return (
        <div className="max-w-7xl mx-auto mt-12 py-12 px-4 sm:py-16 lg:py-20">
            <div className="max-w-4xl mx-auto text-center">
                <div className="text-3xl text-gray-300 sm:text-4xl">
                    Chrome, but more powerful
                </div>
                <p className="mt-3 text-xl text-gray-500 sm:mt-4">
                    Fractal runs Google Chrome on datacenter-grade hardware
                </p>
            </div>
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
