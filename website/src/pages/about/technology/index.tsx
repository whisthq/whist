import React from "react"
import classNames from "classnames"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/about/technology/hero"
import Streaming from "@app/pages/about/technology/streaming"
import Specs from "@app/pages/about/technology/specs"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden"

const Technology = () => {
    return (
        <div id="top">
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Header />
                <Hero />
            </div>
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Streaming />
            </div>
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Specs />
            </div>
            <div className={classNames(padded, "bg-blue-darker")}>
                <Footer />
            </div>
        </div>
    )
}

export default Technology
