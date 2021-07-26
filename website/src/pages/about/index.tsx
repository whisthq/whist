import React from "react"
import classNames from "classNames"

import Header from "@app/shared/components/header"
import Hero from "@app/pages/about/hero"
import Team from "@app/pages/about/team"
import Investors from "@app/pages/about/investors"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden"

export const About = () => {
    return (
        <>
            <div className={classNames(padded, "bg-blue-darker")}>
                <Header />
                <Hero />
            </div>
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Team />
            </div>
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Investors />
            </div>
        </>
    )
}

export default About
