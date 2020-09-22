import React from "react"
import { Button } from "react-bootstrap"
import { connect } from "react-redux"
import firebase from "firebase"

import { logout } from "store/actions/auth/login"

import "styles/auth.css"

const SignoutButton = (props: any) => {
    const handleSignOut = () => {
        firebase
            .auth()
            .signOut()
            .then(() => {
                console.log("signed out")
                props.dispatch(logout())
            })
    }

    return (
        <Button onClick={handleSignOut} className="signout-button">
            Sign out
        </Button>
    )
}

const mapStateToProps = () => {}

export default connect(mapStateToProps)(SignoutButton)
