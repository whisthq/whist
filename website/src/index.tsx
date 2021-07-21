import React from "react"
import ReactDOM from "react-dom"
import { Router } from "react-router"
import history from "@app/shared/utils/history"
import { config } from "@app/shared/constants/config"
import * as Sentry from "@sentry/react"
import RootApp from "@app/pages/root"

import "@app/styles/global.module.css"
import "@app/styles/bootstrap.css"
import { MainProvider } from "@app/shared/utils/context"

if (import.meta.env.FRACTAL_ENVIRONMENT === "production") {
    Sentry.init({
        dsn: "https://4fbefcae900443d58c38489898773eea@o400459.ingest.sentry.io/5394481",
        environment: config.sentry_env,
        release: `website@${import.meta.env.FRACTAL_VERSION as string}`,
    })
}

const RootComponent = () => (
    <Sentry.ErrorBoundary fallback={"An error has occurred"}>
        <Router history={history}>
            <MainProvider>
                <RootApp />
            </MainProvider>
        </Router>
    </Sentry.ErrorBoundary>
)

ReactDOM.render(<RootComponent />, document.getElementById("root"))
