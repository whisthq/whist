import React from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router"

import VerifyView from "pages/auth/pages/verify/components/verifyView"
import { User, AuthFlow } from "shared/types/reducers"

const Verify = (props: { user: User }) => {
    /*
        A wrapper component for the VerifyView component.

        Contains the logic to decide whether to display the /verify page of the website
    
        Arguments:
            user (User): User from Redux state
    */
    const { user } = props

    const search = useLocation().search
    const token = search.substring(1, search.length)

    // logic to process the token if it exists
    const validToken = token && token.length >= 1 ? true : false
    const validUser = user.userID && user.userID !== ""

    // return visuals
    if (!validUser) {
        return <Redirect to="/auth" />
    } else if (user.emailVerified) {
        return <Redirect to="/dashboard" />
    } else {
        return (
            <div>
                <VerifyView token={token} validToken={validToken} />
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(Verify)
