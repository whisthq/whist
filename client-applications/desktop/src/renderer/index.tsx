import React from "react"
/* import { Switch, Route } from "react-router-dom" */
/* import { Router } from "react-router" */
import ReactDOM from "react-dom"

/* import Auth from "@app/renderer/pages/auth/auth" */

/* import { browserHistory } from "@app/utils/history" */

// @ts-ignore
const ipcRenderer = window.ipcRenderer

/* const RootComponent = () => (
*     <>
*         <Router history={browserHistory}>
*             <Switch>
*                 <Route exact path="/" component={Auth} />
*                 <Route path="/auth" component={Auth} />
*             </Switch>
*         </Router>
*     </>
* ) */



const RootComponent = () => {
    return (
        <div className="h-screen w-screen bg-blue-200 flex items-center justify-center">
            <div className="text-red-600">Hey Tulum!</div>
        </div>
    )
}

ReactDOM.render(<RootComponent />, document.getElementById("root"))
