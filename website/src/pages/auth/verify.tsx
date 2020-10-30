import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router"
import { useMutation } from "@apollo/client"

import {
    UPDATE_WAITLIST_AUTH_EMAIL,
    UPDATE_WAITLIST,
} from "shared/constants/graphql"

import * as PureWaitlistAction from "store/actions/waitlist/pure"
import { SIGNUP_POINTS } from "shared/utils/points"

import "styles/auth.css"

import Header from "shared/components/header"
import VerifyView from "pages/auth/views/verifyView"

const Verify = (props: any) => {
    const { user, authFlow, waitlistUser, dispatch } = props
    const [updateWaitlistAuthEmail] = useMutation(UPDATE_WAITLIST_AUTH_EMAIL)
    const [updatePoints] = useMutation(UPDATE_WAITLIST)

    const search = useLocation().search
    const token = search.substring(1, search.length)

    // visual state constants
    const [signupPointsGiven, setSignupPointsGiven] = useState(false)

    // logic to process the token if it exists
    const valid_token = token && token.length >= 1 ? true : false
    const valid_user = user.user_id && user.user_id !== ""

    console.log("THE TOKEN IS")
    console.log(token)

    useEffect(() => {
        if (authFlow.signupSuccess && !signupPointsGiven) {
            updateWaitlistAuthEmail({
                variables: {
                    user_id: waitlistUser.user_id,
                    authEmail: user.user_id,
                },
                optimisticResponse: true,
            })
            updatePoints({
                variables: {
                    user_id: waitlistUser.user_id,
                    points: waitlistUser.points + SIGNUP_POINTS,
                    referrals: waitlistUser.referrals,
                },
            })
            dispatch(
                PureWaitlistAction.updateWaitlistUser({
                    authEmail: user.user_id,
                })
            )
            setSignupPointsGiven(true)
        }
    }, [
        authFlow.signupSuccess,
        updateWaitlistAuthEmail,
        updatePoints,
        dispatch,
        user,
        waitlistUser,
        signupPointsGiven,
    ])

    // return visuals
    if (!valid_user || user.emailVerified) {
        return <Redirect to="/" />
    } else {
        return (
            <div className="fractalContainer">
                <Header color="black" />
                <VerifyView token={token} validToken={valid_token} />
            </div>
        )
    }
}

function mapStateToProps(state: {
    AuthReducer: { user: any; authFlow: any }
    WaitlistReducer: { waitlistUser: any }
}) {
    return {
        user: state.AuthReducer.user ? state.AuthReducer.user : {},
        authFlow: state.AuthReducer.authFlow ? state.AuthReducer.authFlow : {},
        waitlistUser: state.WaitlistReducer.waitlistUser,
    }
}

export default connect(mapStateToProps)(Verify)
