import React, { useEffect, useCallback, useContext } from "react"
import { connect } from "react-redux"

import { db } from "shared/utils/firebase"
import MainContext from "shared/context/mainContext"

import {
    updateUserAction,
    updateWaitlistAction,
} from "store/actions/auth/waitlist"

import { getSortedLeaderboard } from "shared/utils/points"

import "styles/landing.css"
import "styles/shared.css"

import TopView from "pages/landing/views/topView"
import MiddleView from "pages/landing/views/middleView"
import LeaderboardView from "pages/landing/views/leaderboardView"
import BottomView from "pages/landing/views/bottomView"
import Footer from "shared/components/footer"

const Landing = (props: any) => {
    const { setReferralCode, setAppHighlight } = useContext(MainContext)
    const { dispatch, user, match } = props

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
                if (waitlist[i].email === user.email) {
                    return i + 1
                }
            }
            return 0
        },
        [user]
    )

    useEffect(() => {
        db.collection("metadata")
            .doc("waitlist")
            .onSnapshot(function (snapshot) {
                console.log("leaderboard update detected")
                getSortedLeaderboard(snapshot.data()).then(function (
                    sortedLeaderboard
                ) {
                    dispatch(updateWaitlistAction(sortedLeaderboard))
                    if (user && user.email) {
                        const ranking = getRanking(sortedLeaderboard)
                        if (ranking !== user.ranking) {
                            dispatch(updateUserAction(user.points, ranking))
                        }
                    }
                })
            })
    }, [user, dispatch, getRanking])

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

const mapStateToProps = (state: { AuthReducer: { user: any } }) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Landing)
