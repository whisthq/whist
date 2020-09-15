import React, { useEffect, useCallback } from "react"
import { connect } from "react-redux"
import { Row, Col } from "react-bootstrap"

import { db } from "utils/firebase"
import * as firebase from "firebase"

import {
    updateUserAction,
    updateWaitlistAction,
} from "store/actions/auth/waitlist"

import "styles/landing.css"
import "styles/shared.css"

import Leaderboard from "pages/landing/components/leaderboard"
import Actions from "pages/landing/components/actions"

import TopView from "pages/landing/views/topView"
import MiddleView from "pages/landing/views/middleView"
import BottomView from "pages/landing/views/bottomView"

function Landing(props: any) {
    const { dispatch, user } = props

    const getRanking = useCallback(
        (waitlist: any[]) => {
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
        var unsubscribe: any

        const updateRanking = (
            userData: firebase.firestore.DocumentData | undefined,
            waitlist: any[]
        ) => {
            // Bubbles user up the leaderboard according to new points
            if (userData) {
                for (
                    let currRanking = user.ranking - 1;
                    currRanking > 0 &&
                    userData.points >= waitlist[currRanking - 1].points;
                    currRanking--
                ) {
                    if (userData.email > waitlist[currRanking - 1].email) {
                        return currRanking + 1
                    }
                }
            }
            return -1
        }

        getWaitlist()
            .then((waitlist) => {
                dispatch(updateWaitlistAction(waitlist))
                if (user.email) {
                    const ranking = getRanking(waitlist)
                    if (ranking !== user.ranking) {
                        dispatch(updateUserAction(user.points, ranking))
                    }
                }
                return waitlist
            })
            .then((waitlist) => {
                if (user && user.email) {
                    unsubscribe = db
                        .collection("waitlist")
                        .doc(user.email)
                        .onSnapshot(
                            (doc: any) => {
                                const userData = doc.data()
                                const ranking = updateRanking(
                                    userData,
                                    waitlist
                                )
                                console.log(ranking)
                                if (
                                    userData &&
                                    userData.points !== user.points &&
                                    ranking !== -1
                                ) {
                                    dispatch(
                                        updateUserAction(
                                            userData.points,
                                            ranking
                                        )
                                    )
                                }
                            },
                            (err: Error) => console.log(err)
                        )
                }
            })
        return unsubscribe
    }, [dispatch, user, getRanking])

    async function getWaitlist() {
        const waitlist = await db
            .collection("waitlist")
            .orderBy("points", "desc")
            .orderBy("email")
            .get()
        return waitlist.docs.map((doc: any) => doc.data())
    }

    return (
        <div style={{ paddingBottom: 100 }}>
            <TopView />
            <MiddleView />
            <div
                style={{
                    padding: 30,
                    marginTop: 100,
                    background: "#0d1d3c",
                    minHeight: "100vh",
                }}
            >
                <Row>
                    <Col md={6}>
                        <Leaderboard />
                    </Col>
                    <Col md={6}>
                        <Actions />
                    </Col>
                </Row>
            </div>
            <BottomView />
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Landing)
