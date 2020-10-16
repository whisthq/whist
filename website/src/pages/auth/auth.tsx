import React, { useEffect } from "react"
import { connect } from "react-redux"
import { debugLog } from "shared/utils/logging"
import "styles/auth.css"

import GoogleButton from "pages/auth/googleButton"
import SignoutButton from "pages/auth/signoutButton"

const Auth = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    useEffect(() => {
        debugLog(props)
    }, [props])

    return (
        <div className="auth-wrapper">
            <div>email: {props.user.user_id}</div>
            <GoogleButton />
            <SignoutButton />
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Auth)
