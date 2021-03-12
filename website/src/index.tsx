import React from "react"
import ReactDOM from "react-dom"
import { Router } from "react-router"
import history from "@app/shared/utils/history"
import { config } from "@app/shared/constants/config"
import * as Sentry from "@sentry/react"
import RootApp from "@app/pages/root"

import "@app/styles/global.module.css"
import "@app/styles/bootstrap.css"
import "@app/styles/tailwind.css"

if (import.meta.env.REACT_APP_ENVIRONMENT === "production") {
    // the netlify build command is:
    // export REACT_APP_VERSION=$COMMIT_HASH && npm run build
    // so this environment variable should be set on netlify deploys
    Sentry.init({
        dsn:
            "https://4fbefcae900443d58c38489898773eea@o400459.ingest.sentry.io/5394481",
        environment: config.sentry_env,
        release: "website@" + import.meta.env.REACT_APP_VERSION || "local",
    })
}

const RootComponent = () => (
    <Sentry.ErrorBoundary fallback={"An error has occurred"}>
        <Router history={history}>
            <RootApp/>
        </Router>
    </Sentry.ErrorBoundary>
)



ReactDOM.render(<RootComponent />, document.getElementById("root"))
