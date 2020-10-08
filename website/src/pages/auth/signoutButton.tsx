import React from "react"
import { Button } from "react-bootstrap"
import { connect } from "react-redux"
import firebase from "firebase"
import {debug_log} from "shared/utils/logging"
import { logout } from "store/actions/auth/login_actions"

import "styles/auth.css"

const SignoutButton = (props: any) => {
    const handleSignOut = () => {
        firebase
            .auth()
            .signOut()
            .then(() => {
                debug_log("signed out")
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
