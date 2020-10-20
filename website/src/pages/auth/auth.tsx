import React, { useEffect, useState } from "react"
import { connect } from "react-redux"

import LoginView from "pages/auth/views/loginView"

import "styles/auth.css"

const Auth = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const [mode, setMode] = useState("login")

    return <div>{mode === "login" ? <LoginView /> : <div>signup</div>}</div>
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Auth)
