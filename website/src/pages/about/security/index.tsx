import React from "react"
import classNames from "classnames"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/about/security/hero"
import Overview from "@app/pages/about/security/overview"
import Detailed from "@app/pages/about/security/detailed"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden"

export default function Example() {
    return (
        <div>
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Header />
                <Hero />
                <Overview />
                <Detailed />
            </div>
            <div className={classNames(padded, "bg-blue-darker")}>
                <Footer />
            </div>
        </div>
    )
}
