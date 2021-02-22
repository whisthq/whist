import React from "react"
import { Switch, Route } from "react-router-dom"

import Chrome from "pages/homepage/products/chrome/chrome"
import withTracker from "shared/utils/withTracker"

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
