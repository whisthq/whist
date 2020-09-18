import React, { useState, useEffect, useContext } from "react"
import { connect } from "react-redux"
import Countdown from "react-countdown"

import ScreenContext from "shared/context/screenContext"
import { db } from "shared/utils/firebase"

import "styles/landing.css"

const CountdownTimer = (props: any) => {
    const { width } = useContext(ScreenContext)

    const [closingDate, changeClosingDate] = useState(() => {
        return Date.now()
    })

    useEffect(() => {
        getCloseDate().then((closingDate) => {
            changeClosingDate(closingDate)
        })
    }, [])

    const getCloseDate = async () => {
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
                    color: "#00D4FF",
                    fontSize: width > 720 ? 18 : 12,
                    background: props.background ? props.background : "none",
                    padding: props.padding ? props.padding : "8px 15px",
                    borderRadius: 4,
                    border: props.border ? props.border : "solid 1px #00D4FF",
                    textAlign: "center",
                }}
            >
                <strong>{days}</strong>{" "}
                <span style={{ fontSize: width > 720 ? 12 : 9 }}>
                    {" "}
                    days &nbsp;{" "}
                </span>{" "}
                <strong>{hours}</strong>{" "}
                <span style={{ fontSize: width > 720 ? 12 : 10 }}> hrs </span>{" "}
                &nbsp; <strong>{minutes}</strong>{" "}
                <span style={{ fontSize: width > 720 ? 12 : 10 }}> mins </span>{" "}
                &nbsp; <strong>{seconds}</strong>{" "}
                <span style={{ fontSize: width > 720 ? 12 : 10 }}> secs </span>
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
                    fontSize: 140,
                    background: "none",
                    padding: "30px 50px",
                    borderRadius: 2,
                    textAlign: "center",
                    display: "flex",
                    justifyContent: "center",
                    alignItems: "space-between",
                }}
            >
                <div style={{ marginRight: 40 }}>
                    <strong>{days}</strong>{" "}
                    <div style={{ fontSize: 18 }}>days</div>
                </div>
                <div style={{ marginLeft: 40, marginRight: 40 }}>
                    <strong>{hours}</strong>{" "}
                    <div style={{ fontSize: 18 }}>hours</div>
                </div>
                <div style={{ marginLeft: 40, marginRight: 40 }}>
                    <strong>{minutes}</strong>{" "}
                    <div style={{ fontSize: 18 }}>mins</div>
                </div>
                <div style={{ marginLeft: 40 }}>
                    <strong>{seconds}</strong>{" "}
                    <div style={{ fontSize: 18 }}>secs</div>
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

    if (props.type === "small") {
        return (
            <div>
                <Countdown
                    date={closingDate}
                    renderer={smallCountdownRenderer}
                />
            </div>
        )
    } else {
        return (
            <div>
                <Countdown date={closingDate} renderer={bigCountdownRenderer} />
            </div>
        )
    }
}

const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(CountdownTimer)
