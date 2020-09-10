import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { db } from "utils/firebase";

import {
    updateUserAction,
    updateWaitlistAction,
} from "store/actions/auth/waitlist";

import "styles/landing.css";
import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";

import Leaderboard from "pages/landing/components/leaderboard";

function Landing(props: any) {
    const [waitlist, setWaitlist] = useState(props.waitlist);

    useEffect(() => {
        console.log("use effect landing");
        console.log(props.user);
        var unsubscribe;
        getWaitlist()
            .then((waitlist) => {
                setWaitlist(waitlist);
                props.dispatch(updateWaitlistAction(waitlist));
                if (props.user.email && props.user.ranking == 0) {
                    const ranking = getInitialRanking(waitlist);
                    props.dispatch(
                        updateUserAction(props.user.points, ranking)
                    );
                }
            })
            .then(() => {
                if (props.user && props.user.email) {
                    unsubscribe = db
                        .collection("waitlist")
                        .doc(props.user.email)
                        .onSnapshot((doc) => {
                            console.log("IN SNAPSHOT");
                            const userData = doc.data();
                            const ranking = updateRanking(userData);
                            console.log(ranking);
                            if (
                                userData &&
                                userData.points != props.user.points
                            ) {
                                props.dispatch(
                                    updateUserAction(userData.points, ranking)
                                );
                            }
                        });
                }
            });
        return unsubscribe;
    }, [props.user]);

    async function getWaitlist() {
        const waitlist = await db
            .collection("waitlist")
            .orderBy("points", "desc")
            .get();
        return waitlist.docs.map((doc) => doc.data());
    }

    function getInitialRanking(waitlist) {
        for (var i = 0; i < waitlist.length; i++) {
            if (waitlist[i].email == props.user.email) {
                return i + 1;
            }
        }
        return 0;
    }

    function updateRanking(userData) {
        var currRanking = props.user.ranking - 1;
        // Bubbles user up the leaderboard according to new points
        while (
            currRanking > 0 &&
            userData.points > props.waitlist[currRanking - 1].points
        ) {
            setWaitlist((originalWaitlist) => {
                var newWaitlist = [...originalWaitlist];
                newWaitlist[currRanking] = originalWaitlist[currRanking - 1];
                newWaitlist[currRanking - 1] = userData;
                return newWaitlist;
            });
            currRanking--;
        }
        return currRanking + 1;
    }

    return (
        <div
            style={{
                textAlign: "center",
            }}
        >
            <WaitlistForm />
            <CountdownTimer />
            <Leaderboard />
        </div>
    );
}

function mapStateToProps(state) {
    return {
        user: state.AuthReducer.user,
        waitlist: state.AuthReducer.waitlist,
    };
}

export default connect(mapStateToProps)(Landing);
