import React from "react"
import classNames from "classnames"

import Header from "@app/shared/components/header"
import Footer from "@app/shared/components/footer"
import Questions from "@app/pages/about/faq/questions"
import Support from "@app/pages/about/faq/support"

const padded = "pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden"

const FAQ = () => {
  return (
    <>
      <div className={classNames(padded, "bg-blue-darkest")}>
        <Header />
        <Questions />
        <Support />
      </div>
      <Footer />
    </>
  )
}

export default FAQ
