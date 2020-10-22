import React from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"

import Header from "shared/components/header"
import ResetView from "pages/auth/views/resetView"

import "styles/auth.css"

const Reset = (props: {
    dispatch: any
    authFlow: {
        resetTokenStatus: string | null
        resetDone: boolean
    }
}) => {
    const { dispatch, authFlow } = props

    const search = useLocation().search
    const token = search.substring(1, search.length)
    const valid_token = token && token.length >= 1

    if (!valid_token || authFlow.resetDone) {
        return <Redirect to="/" />
    } else {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <ResetView token={token} validToken={valid_token} />
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
    }
}

export default connect(mapStateToProps)(Reset)
