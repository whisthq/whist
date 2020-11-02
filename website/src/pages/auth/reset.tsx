import React from "react"
//import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"

import Header from "shared/components/header"
import ResetView from "pages/auth/views/resetView"

import "styles/auth.css"

const Reset = (props: any) => {
    const search = useLocation().search
    const token = search.substring(1, search.length)
    const valid_token = token && token.length >= 1

    if (!valid_token) {
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

export default Reset
