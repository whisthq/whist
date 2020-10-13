import React, { useEffect, useCallback, useContext } from "react"
import { connect } from "react-redux"
import { useSubscription } from "@apollo/client"

import MainContext from "shared/context/mainContext"
import { SUBSCRIBE_WAITLIST } from "pages/landing/constants/graphql"

import {
    updateUserAction,
    updateWaitlistAction,
    updateApplicationRedirect,
} from "store/actions/auth/waitlist"

import "styles/landing.css"
import "styles/shared.css"

import history from "shared/utils/history"

import TopView from "pages/landing/views/topView"
import MiddleView from "pages/landing/views/middleView"
import LeaderboardView from "pages/landing/views/leaderboardView"
import BottomView from "pages/landing/views/bottomView"
import Footer from "shared/components/footer"

const Landing = (props: any) => {
    const { setReferralCode, setAppHighlight } = useContext(MainContext)
    const { dispatch, user, match, applicationRedirect } = props

    const { data } = useSubscription(SUBSCRIBE_WAITLIST)

    const apps = ["Photoshop", "Blender", "Figma", "VSCode", "Chrome", "Maya"]
    const appsLowercase = [
        "photoshop",
        "blender",
        "figma",
        "vscode",
        "chrome",
        "maya",
    ]

    const getRanking = useCallback(
        (waitlist: any) => {
            for (var i = 0; i < waitlist.length; i++) {
                if (waitlist[i].user_id === user.user_id) {
                    return i + 1
                }
            }
            return 0
        },
        [user]
    )

    useEffect(() => {
        dispatch(updateApplicationRedirect(false))
    }, [])

    useEffect(() => {
        if (data) {
            const waitlist = data.waitlist
            dispatch(updateWaitlistAction(waitlist))
            if (user && user.user_id) {
                const ranking = getRanking(waitlist)
                console.log("get ranking")
                if (ranking !== user.ranking || user.ranking === 0) {
                    dispatch(updateUserAction(user.points, ranking))
                    if (applicationRedirect) {
                        console.log("pushing redirect")
                        history.push("/application")
                    }
                }
            }
        }
    }, [data])

    useEffect(() => {
        const firstParam = match.params.first
        const secondParam = match.params.second
        const idx = appsLowercase.indexOf(firstParam)
        if (idx !== -1) {
            setAppHighlight(apps[idx])
            if (secondParam) {
                setReferralCode(secondParam)
            }
        } else if (firstParam) {
            setReferralCode(firstParam)
        }
    }, [match, apps, appsLowercase, setAppHighlight, setReferralCode])

    return (
        <div>
            <div className="fractalContainer">
                <TopView />
                <MiddleView />
                <LeaderboardView />
                <BottomView />
            </div>
            <Footer />
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: any; applicationRedirect: boolean }
}) => {
    return {
        user: state.AuthReducer.user,
        applicationRedirect: state.AuthReducer.applicationRedirect,
    }
}

export default connect(mapStateToProps)(Landing)
