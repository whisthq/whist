import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Redirect, useLocation } from "react-router"
import { useMutation } from "@apollo/client"

import { UPDATE_WAITLIST_AUTH_EMAIL } from "shared/constants/graphql"

import * as PureWaitlistAction from "store/actions/waitlist/pure"

import "styles/auth.css"

import Header from "shared/components/header"
import VerifyView from "pages/auth/views/verifyView"

const Verify = (props: any) => {
    const { user, authFlow, waitlistUser, dispatch } = props

    const [completed, setCompleted] = useState(false)
    const [error, setError] = useState(false)

    const [updateWaitlistAuthEmail] = useMutation(UPDATE_WAITLIST_AUTH_EMAIL, {
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
        onCompleted: () => {
            setCompleted(true)
        },
        onError: () => {
            setError(true)
        },
    })

    const search = useLocation().search
    const token = search.substring(1, search.length)

    // logic to process the token if it exists
    const validToken = token && token.length >= 1 ? true : false
    const validUser = user.userID && user.userID !== ""

    useEffect(() => {
        if (
            authFlow.signupSuccess &&
            !waitlistUser.authEmail &&
            waitlistUser.userID
        ) {
            updateWaitlistAuthEmail({
                variables: {
                    userID: waitlistUser.userID,
                    authEmail: user.userID,
                },
                optimisticResponse: true,
            })
        }
    }, [authFlow.signupSuccess, updateWaitlistAuthEmail, user, waitlistUser])

    useEffect(() => {
        // update redux with authEmail if successfully updated database
        if (completed && !error) {
            dispatch(
                PureWaitlistAction.updateWaitlistUser({
                    authEmail: user.userID,
                })
            )
        }
    }, [completed, error, dispatch, user.userID])

    // return visuals
    if (!validUser) {
        return <Redirect to="/" />
    } else if (user.emailVerified) {
        return <Redirect to="/dashboard" />
    } else {
        return (
            <div className="fractalContainer">
                <Header dark={false} />
                <VerifyView token={token} validToken={validToken} />
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
