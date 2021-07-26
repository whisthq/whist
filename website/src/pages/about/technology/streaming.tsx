import React from "react"

import {
    FractalButton,
    FractalButtonState,
} from "@app/pages/home/components/button"

const Streaming = () => (
    <div className="py-16 overflow-hidden">
        <div className="max-w-7xl mx-auto space-y-8">
            <div className="text-base max-w-prose mx-auto lg:max-w-none">
                <p className="mt-2 text-3xl leading-8 tracking-tight text-gray-300 sm:text-4xl">
                    How Fractal Works
                </p>
            </div>
            <div className="relative z-10 text-base max-w-prose lg:max-w-5xl lg:mx-0 lg:pr-72">
                <p className="text-lg text-gray-500">
                    Sagittis scelerisque nulla cursus in enim consectetur quam.
                    Dictum urna sed consectetur neque tristique pellentesque.
                    Blandit amet, sed aenean erat arcu morbi. Cursus faucibus
                    nunc nisl netus morbi vel porttitor vitae ut. Amet vitae
                    fames senectus vitae.
                </p>
            </div>
            <div className="lg:grid lg:grid-cols-2 lg:gap-8 lg:items-start">
                <div className="relative z-10">
                    <div className="prose prose-indigo text-gray-500 mx-auto lg:max-w-none md:pr-8">
                        <p>
                            Sollicitudin tristique eros erat odio sed vitae,
                            consequat turpis elementum. Lorem nibh vel, eget
                            pretium arcu vitae. Eros eu viverra donec ut
                            volutpat donec laoreet quam urna.
                        </p>
                        <p>
                            Rhoncus nisl, libero egestas diam fermentum dui. At
                            quis tincidunt vel ultricies. Vulputate aliquet
                            velit faucibus semper. Pellentesque in venenatis
                            vestibulum consectetur nibh id. In id ut tempus
                            egestas. Enim sit aliquam nec, a. Morbi enim
                            fermentum lacus in. Viverra.
                        </p>
                    </div>
                    <div className="mt-12 text-base max-w-prose mx-auto lg:max-w-none">
                        <FractalButton
                            contents="DOWNLOAD NOW"
                            state={FractalButtonState.DEFAULT}
                        />
                    </div>
                </div>
                <div className="mt-12 relative text-base max-w-prose mx-auto lg:mt-0 lg:max-w-none">
                    <blockquote className="relative bg-gray-800 rounded-lg">
                        <div className="rounded-t-lg px-6 py-8 sm:px-10 sm:pt-10 sm:pb-8">
                            <div className="relative text-lg text-gray-400 font-medium mt-8">
                                <svg
                                    className="absolute top-0 left-0 transform -translate-x-3 -translate-y-2 h-8 w-8 text-gray-900"
                                    fill="currentColor"
                                    viewBox="0 0 32 32"
                                    aria-hidden="true"
                                >
                                    <path d="M9.352 4C4.456 7.456 1 13.12 1 19.36c0 5.088 3.072 8.064 6.624 8.064 3.36 0 5.856-2.688 5.856-5.856 0-3.168-2.208-5.472-5.088-5.472-.576 0-1.344.096-1.536.192.48-3.264 3.552-7.104 6.624-9.024L9.352 4zm16.512 0c-4.8 3.456-8.256 9.12-8.256 15.36 0 5.088 3.072 8.064 6.624 8.064 3.264 0 5.856-2.688 5.856-5.856 0-3.168-2.304-5.472-5.184-5.472-.576 0-1.248.096-1.44.192.48-3.264 3.456-7.104 6.528-9.024L25.864 4z" />
                                </svg>
                                <p className="relative">
                                    Tincidunt integer commodo, cursus etiam
                                    aliquam neque, et. Consectetur pretium in
                                    volutpat, diam. Montes, magna cursus nulla
                                    feugiat dignissim id lobortis amet. Laoreet
                                    sem est phasellus eu proin massa, lectus.
                                </p>
                            </div>
                        </div>
                        <cite className="relative flex items-center sm:items-start bg-blue rounded-b-lg not-italic py-5 px-6 sm:py-5 sm:pl-12 sm:pr-10 sm:mt-10">
                            <div className="relative rounded-full border-2 border-white sm:absolute sm:top-0 sm:transform sm:-translate-y-1/2">
                                <img
                                    className="w-12 h-12 sm:w-20 sm:h-20 rounded-full bg-indigo-300"
                                    src="https://images.unsplash.com/photo-1500917293891-ef795e70e1f6?ixlib=rb-1.2.1&auto=format&fit=facearea&facepad=2.5&w=160&h=160&q=80"
                                    alt=""
                                />
                            </div>
                            <span className="relative ml-4 text-indigo-300 font-semibold leading-6 sm:ml-24 sm:pl-1">
                                <p className="text-white font-semibold sm:inline">
                                    Judith Black
                                </p>{" "}
                                <p className="sm:inline">CEO at Workcation</p>
                            </span>
                        </cite>
                    </blockquote>
                </div>
            </div>
        </div>
    </div>
)

export default Streaming
