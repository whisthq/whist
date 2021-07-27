import React from "react"
import classNames from "classnames"
import { Switch, Route } from "react-router-dom"

import withTracker from "@app/shared/utils/withTracker"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/home/hero"
import Benefits from "@app/pages/home/benefits"
import Features from "@app/pages/home/features"
import Download from "@app/pages/home/download"
import Users from "@app/pages/home/users"
import Pricing from "@app/pages/home/pricing"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden dark"

export const Home = () => {
    /*
        Product page for Chrome

        Arguments: none
    */
    return (
        <>
            <div className={classNames(padded, "bg-blue-darkest")}>
                <Header />
                <Hero />
                <Features />
            </div>
            <div className={classNames(padded, "bg-blue-darker")}>
                <Users />
            </div>
            <div className={classNames(padded, "bg-blue-darkest")} id="pricing">
                <Benefits />
                <Pricing />
            </div>
            <div
                className={classNames(padded, "bg-blue-darkest")}
                id="download"
            >
                <Download />
            </div>
            <div className={classNames(padded, "bg-blue-darker")}>
                <Footer />
            </div>
        </>
    )
}

const Router = () => {
    /*
        Sub-router for home page
        Arguments: none
    */

    return (
        <div>
            <Switch>
                <Route exact path="/" component={withTracker(Home)} />
            </Switch>
        </div>
    )
}

export default Router
