import React, { useState, useEffect, useContext, Dispatch } from "react"
import { connect } from "react-redux"
import Countdown from "react-countdown"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"
import { db } from "shared/utils/firebase"

import "styles/landing.css"

import * as PureWaitlistAction from "store/actions/waitlist/pure"

const CountdownTimer = (props: { dispatch: Dispatch<any> }) => {
    const { width } = useContext(MainContext)
    const { dispatch } = props

    const [closingDate, changeClosingDate] = useState(() => {
        return Date.now()
    })

    useEffect(() => {
        getCloseDate().then((date) => {
            changeClosingDate(date)
            dispatch(
                PureWaitlistAction.updateWaitlistData({
                    closingDate: date,
                })
            )
        })
    }, [dispatch])

    async function getCloseDate() {
        const closingDate = await db
            .collection("metadata")
            .doc("waitlist")
            .get()
        const data = closingDate.data()
        return data ? data.close_date.seconds * 1000 : 0
    }

    const smallCountdown = (
        days: number,
        hours: number,
        minutes: number,
        seconds: number
    ): any => {
        return (
            <div
                style={{
                    color: "#111111",
                    fontSize: width > 720 ? 18 : 12,
                    background: "none",
                    textAlign: "center",
                }}
            >
                <strong>{days}</strong>{" "}
                <span style={{ fontSize: 10 }}> days &nbsp; </span>{" "}
                <strong>{hours}</strong>{" "}
                <span style={{ fontSize: 11 }}> hrs </span> &nbsp;{" "}
                <strong>{minutes}</strong>{" "}
                <span style={{ fontSize: 11 }}> mins </span> &nbsp;{" "}
                <span style={{ color: "#3930b8", fontWeight: "bold" }}>
                    {seconds}
                </span>{" "}
                <span style={{ fontSize: 11 }}> secs </span>
            </div>
        )
    }

    const bigCountdown = (
        days: number,
        hours: number,
        minutes: number,
        seconds: number
    ): any => {
        return (
            <div
                style={{
                    color: "#111111",
                    fontSize: 50,
                    background: "none",
                    padding: "30px 50px",
                    borderRadius: 2,
                    textAlign: "center",
                    display: "flex",
                    justifyContent: "center",
                }}
            >
                <div
                    style={{
                        display: "flex",
                        justifyContent: "space-between",
                        width: "100%",
                        maxWidth: 600,
                    }}
                >
                    <div>
                        {days}
                        <div style={{ fontSize: 16 }}>days</div>
                    </div>
                    <div>
                        {hours}
                        <div style={{ fontSize: 16 }}>hours</div>
                    </div>
                    <div>
                        {minutes}
                        <div style={{ fontSize: 16 }}>mins</div>
                    </div>
                    <div>
                        {seconds}
                        <div style={{ fontSize: 16 }}>secs</div>
                    </div>
                </div>
            </div>
        )
    }

    const smallCountdownRenderer = (props: {
        days: any
        hours: any
        minutes: any
        seconds: any
        completed: any
    }) => {
        if (Number(props.seconds) < 10) {
            props.seconds = props.seconds.toString()
        }
        return smallCountdown(
            props.completed ? 0 : props.days,
            props.completed ? 0 : props.hours,
            props.completed ? 0 : props.minutes,
            props.completed ? 0 : props.seconds
        )
    }

    const bigCountdownRenderer = (props: {
        days: any
        hours: any
        minutes: any
        seconds: any
        completed: any
    }) => {
        // Render a countdown
        if (Number(props.seconds) < 10) {
            props.seconds = props.seconds.toString()
        }
        return bigCountdown(
            props.completed ? 0 : props.days,
            props.completed ? 0 : props.hours,
            props.completed ? 0 : props.minutes,
            props.completed ? 0 : props.seconds
        )
    }

    return (
        <div>
            <Countdown
                date={closingDate}
                renderer={
                    width > ScreenSize.SMALL
                        ? bigCountdownRenderer
                        : smallCountdownRenderer
                }
            />
        </div>
    )
}

function mapStateToProps() {
    return {}
}

export default connect(mapStateToProps)(CountdownTimer)
