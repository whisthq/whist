import React from "react"

const metrics = [
    {
        id: 1,
        stat: "300 MB",
        text: "RAM usage",
    },
    {
        id: 2,
        stat: "<5%",
        text: "CPU usage on idle",
    },
    {
        id: 3,
        stat: "<20 ms",
        text: "Latency introduced by Fractal",
    },
    {
        id: 4,
        stat: "60+",
        text: "Frames per second",
    },
]

const Streaming = () => (
    <div className="py-16 overflow-hidden" id="details">
        <div className="max-w-7xl mx-auto space-y-8">
            <div className="text-base max-w-prose mx-auto lg:max-w-none">
                <p className="mt-2 text-3xl leading-8 tracking-tight text-gray-300 sm:text-5xl">
                    How Fractal Works
                </p>
            </div>
            <div className="relative z-10 text-base max-w-prose lg:max-w-5xl lg:mx-0 lg:pr-72">
                <p className="text-lg text-gray-500">
                    Fractal works by running Google Chrome in a powerful
                    computer in the cloud and streaming a video of Chrome to
                    your computer. It&lsquo;s like remote desktop, if remote
                    desktop felt like using a native application.
                </p>
            </div>
            <div className="lg:grid lg:grid-cols-2 lg:gap-8 lg:items-start">
                <div className="relative z-10">
                    <div className="prose prose-indigo text-gray-500 mx-auto lg:max-w-none md:pr-8">
                        <p>
                            The core of Fractal is our streaming technology,
                            which we&lsquo;ve spent years perfecting. We obsess
                            over every millisecond of latency and on minimizing
                            memory, CPU, and network usage. Whereas a native
                            install of Google Chrome consumes several gigabytes
                            of memory and a large percentage of your CPU,
                            Fractal uses only a fraction.
                        </p>
                        <p>
                            The end result? No spinning fan or slow page loads,
                            no matter how many tabs or heavy web apps you have
                            open.
                        </p>
                    </div>
                    <div className="mt-24 grid grid-cols-1 gap-y-12 gap-x-6 sm:grid-cols-2 border-t border-gray-600 pt-8">
                        {metrics.map((item) => (
                            <p key={item.id}>
                                <span className="block text-2xl text-mint">
                                    {item.stat}
                                </span>
                                <span className="mt-1 block text-base text-gray-300">
                                    {item.text}
                                </span>
                            </p>
                        ))}
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
                                    I am supposed to say something nice about
                                    Fractal and I will do so sometime this week.
                                </p>
                            </div>
                        </div>
                        <cite className="relative flex items-center sm:items-start bg-blue rounded-b-lg not-italic py-5 px-6 sm:py-5 sm:pl-12 sm:pr-10 sm:mt-10">
                            <div className="relative rounded-full border-2 border-white sm:absolute sm:top-0 sm:transform sm:-translate-y-1/2">
                                <img
                                    className="w-12 h-12 sm:w-20 sm:h-20 rounded-full bg-indigo-300"
                                    src="https://media-exp1.licdn.com/dms/image/C5603AQE5ly63D-E7dA/profile-displayphoto-shrink_400_400/0/1615683999893?e=1632960000&v=beta&t=Z65X7UBzuYTgrLfucjTpTxy9gkqAD9Kr_jl1bY79UMk"
                                    alt=""
                                />
                            </div>
                            <span className="relative text-indigo-300 leading-6 sm:ml-24 sm:pl-1">
                                <p className="text-white sm:inline font-semibold">
                                    Jenny Wang
                                </p>{" "}
                                <p className="sm:inline">Founder at re-inc</p>
                            </span>
                        </cite>
                    </blockquote>
                </div>
            </div>
        </div>
    </div>
)

export default Streaming
