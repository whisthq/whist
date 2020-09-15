import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import Countdown from "react-countdown"

import { db } from "utils/firebase"

import "styles/landing.css"

function CountdownTimer(props: any) {
    const [closingDate, changeClosingDate] = useState(() => {
        return Date.now()
    })

    useEffect(() => {
        getCloseDate().then(function (closingDate) {
            changeClosingDate(closingDate)
        })
    }, [])

    async function getCloseDate() {
        const closingDate = await db
            .collection("metadata")
            .doc("waitlist")
            .get()
        const data = closingDate.data()
        return data ? data.close_date.seconds * 1000 : 0
    }

    const countdownRenderer = (props: {
        days: any
        hours: any
        minutes: any
        seconds: any
        completed: any
    }) => {
        if (props.completed) {
            return (
                <div
                    style={{
                        color: "#00D4FF",
                        fontSize: 18,
                        background: "none",
                        padding: "8px 15px",
                        borderRadius: 2,
                        border: "solid 1px #00D4FF",
                    }}
                >
                    <strong>0</strong>{" "}
                    <span style={{ fontSize: 12 }}> days &nbsp; </span>{" "}
                    <strong>0</strong>{" "}
                    <span style={{ fontSize: 12 }}> hours </span> &nbsp;{" "}
                    <strong>0</strong>{" "}
                    <span style={{ fontSize: 12 }}> mins </span> &nbsp;{" "}
                    <strong>0</strong>{" "}
                    <span style={{ fontSize: 12 }}> secs </span>
                </div>
            )
        } else {
            // Render a countdown
            if (Number(props.seconds) < 10) {
                props.seconds = props.seconds.toString()
            }
            return (
                <div
                    style={{
                        color: "#00D4FF",
                        fontSize: 18,
                        background: "none",
                        padding: "8px 15px",
                        borderRadius: 2,
                        border: "solid 1px #00D4FF",
                    }}
                >
                    <strong>{props.days}</strong>{" "}
                    <span style={{ fontSize: 12 }}> days &nbsp; </span>{" "}
                    <strong>{props.hours}</strong>{" "}
                    <span style={{ fontSize: 12 }}> hours </span> &nbsp;{" "}
                    <strong>{props.minutes}</strong>{" "}
                    <span style={{ fontSize: 12 }}> mins </span> &nbsp;{" "}
                    <strong>{props.seconds}</strong>{" "}
                    <span style={{ fontSize: 12 }}> secs </span>
                </div>
            )
        }
    }

    return (
        <div>
            <Countdown date={closingDate} renderer={countdownRenderer} />
        </div>
    )
}

function mapStateToProps() {
    return {}
}

export default connect(mapStateToProps)(CountdownTimer)
