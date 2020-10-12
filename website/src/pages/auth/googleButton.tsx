import React, { useContext, useCallback } from "react"
import { connect } from "react-redux"
import firebase from "firebase/app"
import { useMutation } from "@apollo/client"

import { UPDATE_WAITLIST } from "shared/constants/graphql"
import { SIGNUP_POINTS } from "shared/utils/points"
import { googleLogin } from "store/actions/auth/login_actions"

import MainContext from "shared/context/mainContext"

import "styles/landing.css"

const GoogleButton = (props: any) => {
    const { width } = useContext(MainContext)
    const { dispatch, user } = props

    const [updatePoints] = useMutation(UPDATE_WAITLIST, {
        onError(err) {
            console.log(err)
        },
    })

    const updatePointsCallback = useCallback(() => {
        updatePoints({
            variables: {
                user_id: user.email,
                points: user.points + SIGNUP_POINTS,
                referrals: user.referrals,
            },
        })
    }, [])

    const handleGoogleLogin = () => {
        if (user.googleAuthEmail) {
            console.log("User already linked google account")
        } else {
            const provider = new firebase.auth.GoogleAuthProvider()

            firebase
                .auth()
                .signInWithPopup(provider)
                .then(async function (result) {
                    if (result && result.user && result.user.email) {
                        const email = user.email

                        updatePointsCallback()

                        dispatch(
                            googleLogin(
                                result.user.email,
                                user.points + SIGNUP_POINTS
                            )
                        )
                    }
                })
                .catch((e) => console.log(e))
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

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(GoogleButton)
