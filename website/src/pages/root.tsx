import React from "react"
import { Route, Switch } from "react-router-dom"

import Company from "@app/pages/about/company"
import Technology from "@app/pages/about/technology"
import Security from "@app/pages/about/security"
import Cookies from "@app/pages/legal/cookies"
import Privacy from "@app/pages/legal/privacy"
import TermsOfService from "@app/pages/legal/tos"
import Landing from "@app/pages/home"
import FAQ from "@app/pages/about/faq"
import Contact from "@app/pages/resources/contact"
import Download from "@app/pages/download"

import routes from "@app/shared/constants/routes"
import withTracker from "@app/shared/utils/withTracker"

const RootApp = () => {
  /*
        Highest-level React component, contains router and ApolloClient
        Arguments:
            accessToken(string): Access token, if any
    */
  return (
    <div className="bg-blue-darkest">
      <Switch>
        <Route exact path={routes.DOWNLOAD} component={Download} />
        <Route exact path={routes.CONTACT} component={Contact} />
        <Route exact path={routes.ABOUT} component={withTracker(Company)} />
        <Route
          exact
          path={routes.TECHNOLOGY}
          component={withTracker(Technology)}
        />
        <Route exact path={routes.FAQ} component={withTracker(FAQ)} />
        <Route exact path={routes.SECURITY} component={withTracker(Security)} />
        <Route exact path={routes.COOKIES} component={withTracker(Cookies)} />
        <Route exact path={routes.PRIVACY} component={withTracker(Privacy)} />
        <Route
          exact
          path={routes.TOS}
          component={withTracker(TermsOfService)}
        />
        <Route exact path={routes.LANDING} component={withTracker(Landing)} />
      </Switch>
    </div>
  )
}

export default RootApp
