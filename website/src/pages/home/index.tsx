import React from 'react'
import { FaMoon, FaSun } from 'react-icons/fa'
import classNames from 'classnames'
import { Switch, Route } from 'react-router-dom'

import withTracker from '@app/shared/utils/withTracker'
import { withContext } from '@app/shared/utils/context'

import Header from '@app/shared/components/header'
import Footer from '@app/shared/components/footer'
import Top from './top'
import Middle from './middle'
import Features from './features'
import ActionPrompt from './bottom'

const DarkModeIcon = (props: { dark: boolean, onClick: () => void }) => (
  <div className="absolute">
    <div
      onClick={props.onClick}
      className={classNames(
        'fixed top-16 md:top-24 right-2 md:right-12 dark:bg-transparent',
        'border border-transparent dark:border-gray-400 bg-blue-lightest',
        'text-gray dark:text-gray-300 p-2 rounded cursor-pointer z-50'
      )}
    >
      <div>{props.dark ? <FaSun /> : <FaMoon />}</div>
    </div>
  </div>
)

export const Chrome = () => {
  /*
        Product page for Chrome

        Arguments: none
    */
  const { dark, setDark } = withContext()

  return (
    <div
      className={classNames(
        'overflow-x-hidden',
        dark ? 'dark bg-blue-darkest' : 'bg-white'
      )}
    >
      <div className="pb-20 px-12 max-w-screen-2xl m-auto overflow-x-hidden">
        <DarkModeIcon dark={dark} onClick={() => setDark(!dark)} />
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
