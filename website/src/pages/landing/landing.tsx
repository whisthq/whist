import React, { useEffect, useCallback, useContext } from "react"
import { connect } from "react-redux"
import { useSubscription } from "@apollo/client"

import MainContext from "shared/context/mainContext"
import { SUBSCRIBE_WAITLIST } from "pages/landing/constants/graphql"

import * as PureWaitlistAction from "store/actions/waitlist/pure"

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
    const { dispatch, user, waitlistUser, match, applicationRedirect } = props

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

    const getUser = useCallback(
        (waitlist: any) => {
            for (var i = 0; i < waitlist.length; i++) {
                if (waitlist[i].user_id === user.user_id) {
                    return {
                        ...waitlist[i],
                        ranking: i + 1,
                        referralCode: waitlist[i].referral_code,
                    }
                }
            }
            return null
        },
        [user]
    )

    useEffect(() => {
        dispatch(
            PureWaitlistAction.updateNavigation({ applicationRedirect: false })
        )
    }, [dispatch])

    useEffect(() => {
        if (data) {
            const waitlist = data.waitlist
            dispatch(
                PureWaitlistAction.updateWaitlistData({ waitlist: waitlist })
            )

            if (user && user.user_id) {
                const newUser = getUser(waitlist)
                if (newUser) {
                    if (
                        newUser.ranking !== user.ranking ||
                        waitlistUser.ranking === 0 ||
                        waitlistUser.points !== newUser.points
                    ) {
                        dispatch(
                            PureWaitlistAction.updateWaitlistUser({
                                points: newUser.points,
                                ranking: newUser.ranking,
                                referralCode: newUser.referralCode,
                            })
                        )

                        if (applicationRedirect) {
                            history.push("/application")
                        }
                    }
                }
            }
        }
    }, [
        data,
        user,
        dispatch,
        applicationRedirect,
        getUser,
        waitlistUser.points,
        waitlistUser.ranking,
    ])

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
    AuthReducer: { user: any }
    WaitlistReducer: { navigation: any; waitlistUser: any }
}) => {
    return {
        user: state.AuthReducer.user,
        waitlistUser: state.WaitlistReducer.waitlistUser,
        applicationRedirect:
            state.WaitlistReducer.navigation.applicationRedirect,
    }
}

export default connect(mapStateToProps)(Landing)
