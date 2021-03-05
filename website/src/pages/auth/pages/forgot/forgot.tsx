import React, {
    useState,
    useEffect,
    KeyboardEvent,
    ChangeEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"
import { Switch, Route } from "react-router-dom"

import EmailForm from "pages/auth/pages/forgot/pages/emailForm/emailForm"
import Reset from "pages/auth/pages/forgot/pages/reset/reset"

import { routeMap, fractalRoute } from "shared/constants/routes"

const Forgot = (props: {
    dispatch: Dispatch<any>
}) => {
    /*
        Component for when the user forgets their login information.

        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
    */
    return(
        <>
            <Switch>
                <Route exact path={fractalRoute(routeMap.AUTH.FORGOT)} component={EmailForm} />
                <Route path={fractalRoute(routeMap.AUTH.FORGOT.RESET)} component={Reset} />
            </Switch>
        </>
    )
}

const mapStateToProps = () => {
    return {
    }
}

export default connect(mapStateToProps)(Forgot)
