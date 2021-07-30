import React from "react"
import classNames from "classnames"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/download/hero"
import Requirements from "@app/pages/download/requirements"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden dark"

const Download = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  return (
    <>
      <div className={classNames(padded, "bg-blue-darkest")} id="top">
        <Header />
        <Hero />
      </div>
      <div className={classNames(padded, "bg-blue-darker")}>
        <Requirements />
      </div>
      <Footer />
    </>
  )
}

export default Download
