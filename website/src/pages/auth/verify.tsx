import React from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router"

import "styles/auth.css"

import Header from "shared/components/header"
import VerifyView from "pages/auth/views/verifyView"

const Verify = (props: { dispatch: any; user: any }) => {
    // we don't use dispatch here
    const { user } = props

    const search = useLocation().search
    const token = search.substring(1, search.length)

    const valid_token = token && token.length >= 1 ? true : false
    const valid_user = user.user_id && user.user_id !== ""

    // return visuals
    if (!valid_user || user.emailVerified) {
        return <Redirect to="/" />
    } else {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <VerifyView token={token} validToken={valid_token} />
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
    }
}

export default connect(mapStateToProps)(Verify)
