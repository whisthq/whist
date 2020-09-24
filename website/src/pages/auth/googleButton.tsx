import React, { useContext } from "react"
import { Button } from "react-bootstrap"
import { connect } from "react-redux"
import firebase from "firebase"
import { db } from "shared/utils/firebase"
import { SIGNUP_POINTS, getUnsortedLeaderboard } from "shared/utils/points"
import { googleLogin } from "store/actions/auth/login_actions"

import MainContext from "shared/context/mainContext"

import "styles/landing.css"

const GoogleButton = (props: any) => {
    const { width } = useContext(MainContext)
    const { dispatch, user } = props

    const handleGoogleLogin = () => {
        if (user.googleAuthEmail) {
            console.log("User already linked google account")
        } else {
            const provider = new firebase.auth.GoogleAuthProvider()

            firebase
                .auth()
                .signInWithPopup(provider)
                .then(async function (result) {
                    const email = user.email
                    if (result && result.user && result.user.email) {
                        const unsortedLeaderboard = await getUnsortedLeaderboard()

                        unsortedLeaderboard[email] = {
                            referrals: unsortedLeaderboard[email].referrals,
                            points:
                                unsortedLeaderboard[email].points +
                                SIGNUP_POINTS,
                            name: unsortedLeaderboard[email].name,
                            email: email,
                            referralCode: unsortedLeaderboard[email]
                                .referralCode
                                ? unsortedLeaderboard[email].referralCode
                                : null,
                            googleAuthEmail: result.user.email,
                        }
                        db.collection("metadata").doc("waitlist").update({
                            leaderboard: unsortedLeaderboard,
                        })
                        dispatch(
                            googleLogin(
                                result.user.email,
                                unsortedLeaderboard[email].points +
                                    SIGNUP_POINTS
                            )
                        )
                    }
                })
                .catch((e) => console.log(e))
        }
    }

    return (
        <Button onClick={handleGoogleLogin} className="action">
            <div
                style={{
                    color: "#111111",
                    fontSize: width > 720 ? 22 : 16,
                }}
            >
                Sign in with Google
            </div>
            <div style={{ color: "#3930b8", fontWeight: "bold" }}>
                {" "}
                +100 points
            </div>
        </Button>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(GoogleButton)
