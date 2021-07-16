import React from "react"
import classNames from "classnames"
import { Switch, Route } from "react-router-dom"

import withTracker from "@app/shared/utils/withTracker"
import { withContext } from "@app/shared/utils/context"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Top from "./top"
import Middle from "./middle"
import Features from "./features"
import ActionPrompt from "./bottom"

export const Chrome = () => {
  /*
        Product page for Chrome

        Arguments: none
    */
  const { dark } = withContext()

  return (
    <div
      className={classNames(
        "overflow-x-hidden",
        dark ? "dark bg-blue-darkest" : "bg-white"
      )}
    >
      <div className="pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden">
        <Header dark={dark} />
        <Top />
        <Middle />
        <Features />
        <ActionPrompt />
      </div>
      <Footer />
    </div>
  )
}

const Products = () => {
  /*
        Sub-router for products page

        Arguments: none
    */

  return (
    <div>
      <Switch>
        <Route exact path="/" component={withTracker(Chrome)} />
        {/* <Route
                    path="/dashboard/settings"
                    component={withTracker(Settings)}
                /> */}
      </Switch>
    </div>
  )
}

export default Products
