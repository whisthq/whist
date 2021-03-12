import React from "react"
import { Link } from "react-router-dom"

export const ActionPrompt = () => {
    /*
        Colorful box component with Get Started prompt
 
        Arguments: 
            none
    */
    return (
        <div className="mt-48 text-center">
            <div className="rounded px-0 md:px-12 py-10 md:py-14">
                <div className="text-4xl md:text-5xl tracking-wide leading-snug text-gray dark:text-gray-300">
                    Give Chrome superpowers.
                </div>
                <div className="font-body mt-4 text-md md:text-lg text-gray tracking-wide dark:text-gray-400">
                    Discover a faster, lighter, and more private Google Chrome.
                </div>
                <Link to="/dashboard">
                    <button className="rounded bg-blue dark:bg-mint px-8 py-3 mt-8 text-gray-100 dark:text-black w-full md:w-48 hover:bg-mint hover:text-black duration-500">
                        GET STARTED
                    </button>
                </Link>
            </div>
        </div>
    )
}

export default ActionPrompt
