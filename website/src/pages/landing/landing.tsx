import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { db } from "utils/firebase";

import { updateWaitlistAction } from "store/actions/auth/waitlist";

import "styles/landing.css";
import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";

import Leaderboard from "pages/leaderboard/leaderboard";

function Landing(props: any) {
    const [waitlist, setWaitlist] = useState(props.waitlist);

    useEffect(() => {
        const unsubscribe = db
            .collection("waitlist")
            .orderBy("points", "desc")
            .onSnapshot((querySnapshot) => {
                props.dispatch(
                    updateWaitlistAction(
                        querySnapshot.docs.map((doc) => doc.data())
                    )
                );
            });
        return unsubscribe;
    }, []);

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
