import React from "react"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Hero from "@app/pages/download/hero"

const Download = () => {
  /*
        Colorful box component with Get Started prompt

        Arguments:
            none
    */
  return (
    <>
      <div
        className="pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden dark bg-gray-900 h-screen"
        id="top"
      >
        <Header />
        <Hero />
      </div>
      <Footer />
    </>
  )
}

export default Download
