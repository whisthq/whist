import React, { useEffect, useState } from "react"
import { connect } from "react-redux"

import "styles/auth.css"

import GoogleButton from "pages/auth/googleButton"
import SignoutButton from "pages/auth/signoutButton"

const Auth = (props: {
    dispatch: (arg0: { type: string; email?: string }) => void
    loggedIn: any
    email: React.ReactNode
}) => {
    useEffect(() => {
        console.log(props)
    }, [props])

    return (
        <div className="auth-wrapper">
            <div>Logged in: {JSON.stringify(props.loggedIn)}</div>
            <div>email: {props.email}</div>
            <GoogleButton />
            <SignoutButton />
        </div>
    )
}

function mapStateToProps(state: any) {
    return {
        reducer: state.AuthReducer,
        loggedIn: state.AuthReducer.logged_in,
        email: state.AuthReducer.user.email,
        name: state.AuthReducer.user.name,
    }
}

export default connect(mapStateToProps)(Auth)
