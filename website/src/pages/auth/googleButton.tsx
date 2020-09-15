import React from "react"
import { Button } from "react-bootstrap"
import { connect } from "react-redux"
import firebase from "firebase"
import { db } from "utils/firebase"
import { SIGNUP_POINTS } from "utils/points"
import { googleLogin } from "store/actions/auth/login_actions"

import "styles/landing.css"

const GoogleButton = (props: any) => {
    const { user } = props

    const handleGoogleLogin = () => {
        if (user.google_auth_email) {
            console.log("User already linked google account")
        } else {
            const provider = new firebase.auth.GoogleAuthProvider()

            firebase
                .auth()
                .signInWithPopup(provider)
                .then((result) => {
                    if (result && result.user && result.user.email) {
                        const email = result.user.email
                        console.log(email)
                        const newPoints = user.points + SIGNUP_POINTS
                        props.dispatch(googleLogin(email, newPoints))
                        db.collection("waitlist")
                            .doc(user.email)
                            .update({
                                google_auth_email: email,
                                points: newPoints,
                            })
                    }
                })
                .catch((e) => console.log(e))
        }
    }

    return (
        <Button onClick={handleGoogleLogin} className="action">
            <div
                style={{
                    color: "white",
                    fontSize: "23px",
                }}
            >
                Sign in with Google
            </div>
            <div style={{ color: "#00D4FF" }}> +100 points</div>
        </Button>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(GoogleButton)
