import React from "react"
import classNames from "classnames"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/about/technology/hero"
import Streaming from "@app/pages/about/technology/streaming"
import Download from "@app/pages/about/technology/download"

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
        <Download />
      </div>
      <Footer />
    </div>
  )
}

export default Technology
