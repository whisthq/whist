import React from "react"
<<<<<<< HEAD
//import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"

=======
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"

import { updateAuthFlow } from "store/actions/auth/pure"

>>>>>>> staging
import Header from "shared/components/header"
import ResetView from "pages/auth/views/resetView"

import "styles/auth.css"

const Reset = (props: any) => {
<<<<<<< HEAD
=======
    const { dispatch } = props
>>>>>>> staging
    const search = useLocation().search
    const token = search.substring(1, search.length)
    const valid_token = token && token.length >= 1

    if (!valid_token) {
<<<<<<< HEAD
        return <Redirect to="/" />
=======
        dispatch(
            updateAuthFlow({
                mode: "Forgot",
            })
        )
        return <Redirect to="/auth/bypass" />
>>>>>>> staging
    } else {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <ResetView token={token} validToken={valid_token} />
            </div>
        )
    }
}

<<<<<<< HEAD
export default Reset
=======
export default connect()(Reset)
>>>>>>> staging
