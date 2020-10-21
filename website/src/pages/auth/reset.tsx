import React from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router-dom"

import "styles/auth.css"

// props type
// {
//     dispatch: any
// }
const Reset = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const search = useLocation().search
    const token = search.substring(1, search.length)

    if (!token || token.length < 1) {
        return <Redirect to="/" />
    } else {
        return <div />
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Reset)
