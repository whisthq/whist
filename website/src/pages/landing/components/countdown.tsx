import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { FormControl, InputGroup, Button } from "react-bootstrap";
import Countdown from "react-countdown"

import { db } from "utils/firebase";

import "styles/landing.css";

function CountdownTimer(props: any) {
    const [closingDate, changeClosingDate] = useState(() => { return (Date.now()) })

    useEffect(() => {
        getCloseDate().then(function (closingDate) {
            changeClosingDate(closingDate)
        })
    }, []);

    async function getCloseDate() {
        const closingDate = await db.collection("metadata").doc("waitlist").get();
        return (closingDate.data().close_date.seconds * 1000)
    }

    const countdownRenderer = ({ days, hours, minutes, seconds, completed }) => {
        if (completed) {
            return <span>done</span>;
        } else {
            // Render a countdown
            if (Number(seconds) < 10) {
                seconds = seconds.toString()
            }
            return (
                <div style={{ color: "#00D4FF", fontSize: 18, background: "none", padding: "8px 15px", borderRadius: 2, border: "solid 1px #00D4FF" }}>
                    <strong>{days}</strong> <span style={{ fontSize: 12 }}> days &nbsp; </span> <strong>{hours}</strong> <span style={{ fontSize: 12 }}> hours </span> &nbsp; <strong>{minutes}</strong> <span style={{ fontSize: 12 }}> mins </span> &nbsp; <strong>{seconds}</strong> <span style={{ fontSize: 12 }}> secs </span>
                </div>
            );
        }
    };

    return (
        <div>
            <Countdown date={closingDate} renderer={countdownRenderer} />
        </div>
    );
}

function mapStateToProps(state) {
    return {
    }
}

export default connect(mapStateToProps)(CountdownTimer);
