import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { FormControl, InputGroup, Button } from "react-bootstrap";

import { db } from "utils/firebase";

import "styles/landing.css";

function Countdown(props: any) {
    useEffect(() => {
        getCloseDate().then(function (closing_date) {
            console.log(closing_date)
        })
    }, []);

    async function getCloseDate() {
        const closing_date = await db.collection("metadata").doc("waitlist").get();
        console.log(closing_date)
    }

    const countdownRenderer = ({ hours, minutes, seconds, completed }) => {
        if (completed) {
            console.log("COUNTDOWN IS OVER")
        } else {
            // Render a countdown
            if (Number(seconds) < 10) {
                seconds = "0" + seconds.toString()
            }
            return <span>{minutes}:{seconds}</span>;
        }
    };

    return (
        <div>
            <Countdown date={Number(999999999) + 60000} renderer={countdownRenderer} />
        </div>
    );
}

function mapStateToProps(state) {
    return {
    }
}

export default connect(mapStateToProps)(Countdown);
