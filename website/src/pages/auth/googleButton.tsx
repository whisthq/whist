import React, { useContext } from "react"
import { connect } from "react-redux"
import firebase from "firebase/app"
import { db } from "shared/utils/firebase"
import { SIGNUP_POINTS } from "shared/utils/points"
import { googleLogin } from "store/actions/auth/login_actions"
import { debugLog } from 'shared/utils/logging'

import MainContext from "shared/context/mainContext"
import "styles/landing.css"

const GoogleButton = (props: any) => {
    const { width } = useContext(MainContext)
    const { dispatch, user, unsortedLeaderboard } = props

    const handleGoogleLogin = () => {
        if (user.googleAuthEmail) {
            debugLog("User already linked google account")
        } else {
            const provider = new firebase.auth.GoogleAuthProvider()

            firebase
                .auth()
                .signInWithPopup(provider)
                .then(async function (result) {
                    const email = user.email
                    if (result && result.user && result.user.email) {
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
                .catch((e) => debugLog(e))
        }
    }

    return (
        <button
            onClick={handleGoogleLogin}
            className="action"
            style={{
                display: "flex",
                justifyContent: "space-between",
                flexDirection: "row",
                width: "100%",
            }}
        >
            <div
                style={{
                    fontSize: width > 720 ? 20 : 16,
                    fontWeight: "bold",
                }}
            >
                Sign in with Google
            </div>
            <div className="points">+100 points</div>
        </button>
    )
}

function mapStateToProps(state: {
    AuthReducer: { user: any; unsortedLeaderboard: any }
}) {
    return {
        user: state.AuthReducer.user,
        unsortedLeaderboard: state.AuthReducer.unsortedLeaderboard
            ? state.AuthReducer.unsortedLeaderboard
            : null,
    }
}

export default connect(mapStateToProps)(GoogleButton)
