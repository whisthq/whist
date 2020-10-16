import React, { useState, useEffect, useContext } from "react"
import { connect } from "react-redux"
import { Row, Col } from "react-bootstrap"
import { FaCrown } from "react-icons/fa"

import Countdown from "pages/landing/components/countdown"
import CountdownTimer from "pages/landing/components/countdown"
import MainContext from "shared/context/mainContext"

import "styles/landing.css"

const Leaderboard = (props: {
    waitlist: any[]
    waitlistUser: any
    user: { user_id: any; ranking: number }
}) => {
    const { width } = useContext(MainContext)
    const [topSix, setTopSix]: any[] = useState([])

    useEffect(() => {
        setTopSix(props.waitlist ? props.waitlist.slice(0, 6) : [])
    }, [props.waitlist])

    const renderRow = (
        idx: number,
        userRow: boolean,
        name: string,
        referrals: number,
        points: number
    ): any => {
        const barWidth =
            props.waitlist && props.waitlist.length > 0 && props.waitlist[0]
                ? Math.round(
                      Math.max((points * 100) / props.waitlist[0].points, 5)
                  )
                : 0

        return (
            <div
                style={{
                    color: "#111111",
                    background: userRow
                        ? "#3930b8"
                        : "rgba(213, 225, 245, 0.2)",
                    padding: "20px 25px",
                    marginBottom: 2,
                }}
            >
                <Row style={{ width: "100%" }}>
                    <Col xs={2}>
                        <div
                            style={{
                                fontSize: 18,
                                fontWeight: "bold",
                                color: userRow ? "white" : "#111111",
                                position: "relative",
                                top: 10,
                            }}
                        >
                            {idx}
                        </div>
                    </Col>
                    <Col
                        xs={6}
                        style={{
                            fontWeight: "bold",
                            color: userRow ? "white" : "#111111",
                            fontSize: 18,
                            display: "flex",
                        }}
                    >
                        {idx === 1 && (
                            <FaCrown
                                style={{
                                    marginRight: 15,
                                    position: "relative",
                                    top: 4,
                                    color: "#3930b8",
                                }}
                            />
                        )}
                        {name}
                    </Col>
                    <Col xs={4} style={{ paddingRight: 0 }}>
                        <div
                            style={{
                                fontSize: 16,
                                textAlign: "right",
                                color: userRow ? "white" : "#111111",
                            }}
                        >
                            <strong>{points}</strong> pts
                        </div>
                    </Col>
                </Row>
                <Row style={{ paddingTop: idx = 3 ? 20 : 0 }}>
                    <Col xs={2}></Col>
                    <Col xs={8} style={{ paddingLeft: 10, display: "flex" }}>
                        <div
                            style={{
                                height: 5,
                                width: barWidth.toString() + "%",
                                background: "#93F1D9",
                            }}
                        ></div>
                        <div
                            style={{
                                height: 5,
                                width: (100 - barWidth).toString() + "%",
                                background: "#e6e8fc",
                            }}
                        ></div>
                    </Col>
                </Row>
            </div>
        )
    }

    const getRows = () => {
        const topThree = topSix.slice(0, 3)
        if (!props.waitlist) {
            return <tr>No data to show.</tr>
        } else if (!props.user.user_id || props.waitlistUser.ranking <= 5) {
            const bottomThree = topSix.slice(3, 6)
            return topThree
                .map((user: any, idx: number) => {
                    return renderRow(
                        idx + 1,
                        idx + 1 === props.waitlistUser.ranking,
                        user.name,
                        user.referrals,
                        user.points
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return renderRow(
                            idx + 4,
                            idx + 4 === props.waitlistUser.ranking,
                            user.name,
                            user.referrals,
                            user.points
                        )
                    })
                )
        } else if (props.waitlistUser.ranking === props.waitlist.length) {
            const bottomThree = props.waitlist.slice(-3)
            return topThree
                .map((user: any, idx: number) => {
                    return renderRow(
                        idx + 1,
                        false,
                        user.name,
                        user.referrals,
                        user.points
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return renderRow(
                            props.waitlistUser.ranking - 2 + idx,
                            idx === 2,
                            user.name,
                            user.referrals,
                            user.points
                        )
                    })
                )
        } else {
            const bottomThree = props.waitlist.slice(
                props.waitlistUser.ranking - 2,
                props.waitlistUser.ranking + 1
            )
            return topThree
                .map((user: any, idx: number) => {
                    return renderRow(
                        idx + 1,
                        false,
                        user.name,
                        user.referrals,
                        user.points
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return renderRow(
                            props.waitlistUser.ranking - 1 + idx,
                            idx === 1,
                            user.name,
                            user.referrals,
                            user.points
                        )
                    })
                )
        }
    }

    return (
        <div className="leaderboard-container">
            <div
                style={{
                    width: "100%",
                    zIndex: 2,
                    paddingTop: width > 720 ? 0 : 20,
                }}
            >
                {width > 720 ? (
                    <div
                        style={{
                            width: "100%",
                            marginTop: 50,
                            paddingRight: 40,
                        }}
                    >
                        <div
                            style={{
                                color: "#111111",
                                background: "rgba(213, 225, 245, 0.2)",
                                padding: "10px 25px",
                                marginBottom: 2,
                            }}
                        >
                            <Countdown />
                        </div>
                        {getRows()}
                    </div>
                ) : (
                    <div style={{ marginBottom: 30, marginTop: 20 }}>
                        <div
                            style={{
                                color: "#111111",
                                background: "rgba(213, 225, 245, 0.2)",
                                padding: "30px 15px",
                                marginBottom: 2,
                            }}
                        >
                            <CountdownTimer type="small" />
                        </div>
                        {getRows()}
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: any }
    WaitlistReducer: {
        waitlistUser: any
        waitlistData: any
    }
}) => {
    console.log(state)
    return {
        user: state.AuthReducer.user,
        waitlist: state.WaitlistReducer.waitlistData.waitlist,
        waitlistUser: state.WaitlistReducer.waitlistUser,
    }
}

export default connect(mapStateToProps)(Leaderboard)
