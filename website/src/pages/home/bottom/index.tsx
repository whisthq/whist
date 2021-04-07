import React from "react"

import Typeform from "@app/pages/home/components/typeform"

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
                <Typeform text="JOIN WAITLIST"/>
            </div>
        </div>
    )
}

export default ActionPrompt
