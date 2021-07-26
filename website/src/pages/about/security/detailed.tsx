import React from "react"

const Detailed = () => {
    return (
        <div className="relative py-16 overflow-hidden mt-24">
            <div className="relative px-4 sm:px-6 lg:px-8">
                <div className="text-lg max-w-prose mx-auto">
                    <h1>
                        <span className="block text-base text-center text-blue tracking-wide uppercase">
                            Technical Spec
                        </span>
                        <span className="mt-3 block text-3xl text-center leading-8 text-gray-300 sm:text-5xl">
                            How We Keep Your Data Private
                        </span>
                    </h1>
                    <p className="mt-8 text-xl text-gray-500 leading-8 text-left">
                        Fractal works by streaming Google Chrome from our
                        datacenters to your computer. Because Google Chrome is
                        being streamed to your computer from a datacenter in the
                        form of video pixels, there are two types of security to
                        consider: security in motion and security at rest.
                    </p>
                </div>
            </div>
        </div>
    )
}

export default Detailed
