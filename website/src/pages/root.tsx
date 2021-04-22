import React from "react";
import { Route, Switch } from "react-router-dom";

import About from "@app/pages/about";
import Cookies from "@app/pages/legal/cookies";
import Privacy from "@app/pages/legal/privacy";
import TermsOfService from "@app/pages/legal/tos";
import Landing from "@app/pages/home";

import routes from "@app/shared/constants/routes";
import withTracker from "@app/shared/utils/withTracker";

const RootApp = () => {
  /*
        Highest-level React component, contains router and ApolloClient
        Arguments:
            accessToken(string): Access token, if any
    */
  return (
    <Switch>
      <Route exact path={routes.ABOUT} component={withTracker(About)} />
      <Route exact path={routes.COOKIES} component={withTracker(Cookies)} />
      <Route exact path={routes.PRIVACY} component={withTracker(Privacy)} />
      <Route exact path={routes.TOS} component={withTracker(TermsOfService)} />
      <Route exact path={routes.LANDING} component={withTracker(Landing)} />
    </Switch>
  );
};

export default RootApp;
