import React from "react";
import { Switch, Route } from "react-router";
import routes from "constants/routes.json";
import App from "containers/App";
import LoginContainer from "containers/Login";
import Dashboard from "containers/Dashboard";
import Studios from "containers/Studios";

export default function Routes() {
  return (
    <App>
      <Switch>
        <Route path={routes.STUDIOS} component={Studios} />
        <Route path={routes.DASHBOARD} component={Dashboard} />
        <Route path={routes.HOME} component={LoginContainer} />
      </Switch>
    </App>
  );
}
