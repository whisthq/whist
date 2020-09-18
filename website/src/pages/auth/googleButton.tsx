import React, { useContext } from "react"
import { Button } from "react-bootstrap"
import { connect } from "react-redux"
import firebase from "firebase"
import { db } from "shared/utils/firebase"
import { SIGNUP_POINTS } from "shared/utils/points"
import { googleLogin } from "store/actions/auth/login"

import ScreenContext from "shared/context/screenContext"

import "styles/landing.css"

const GoogleButton = (props: any) => {
    const { width } = useContext(ScreenContext)
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
                        const newPoints = user.points + SIGNUP_POINTS
                        props.dispatch(googleLogin(email, newPoints))
                        db.collection("waitlist").doc(user.email).update({
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
                    fontSize: width > 720 ? 22 : 16,
                }}
            >
                Sign in with Google
            </div>
            <div style={{ color: "#00D4FF" }}> +100 points</div>
        </Button>
    )
}

const mapStateToProps = (state: { MainReducer: { user: any } }) => {
    return {
        user: state.MainReducer.user,
    }
}

export default connect(mapStateToProps)(GoogleButton)
