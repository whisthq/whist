import React, { useState } from "react"
import { FaMoon, FaSun } from "react-icons/fa"

import Header from "shared/components/header"
import Footer from "shared/components/footer/footer"
import Top from "pages/homepage/products/chrome/components/top/top"
import Middle from "pages/homepage/products/chrome/components/middle/middle"
import Pricing from "pages/homepage/products/chrome/components/pricing/pricing"
import Features from "pages/homepage/products/chrome/components/features/features"
import ActionPrompt from "pages/homepage/products/chrome/components/actionPrompt/actionPrompt"

import sharedStyles from "styles/shared.module.css"

export const Chrome = () => {
    /*
        Product page for Chrome
 
        Arguments: none
    */
    const [dark, setDark] = useState(true)

    return (
        <div
            className={`overflow-x-hidden ${
                dark ? "bg-blue-darkest" : "bg-white"
            } ${dark ? "dark" : ""}`}
        >
            <div className={sharedStyles.fractalContainer}>
                <div className="absolute z-50">
                    <div
                        onClick={() => setDark(!dark)}
                        className="fixed top-16 md:top-24 right-2 md:right-12 dark:bg-transparent border border-transparent dark:border-gray-400 bg-blue-lightest text-gray dark:text-gray-300 p-2 rounded cursor-pointer"
                    >
                        {dark ? (
                            <div>
                                <FaSun />
                            </div>
                        ) : (
                            <div>
                                <FaMoon />
                            </div>
                        )}
                    </div>
                </div>
                <Header dark={dark} />
                <Top />
                <Middle />
                <Pricing dark={dark} />
                <Features />
                <ActionPrompt />
            </div>
            <Footer />
        </div>
    )
}

export default Chrome
