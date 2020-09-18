import React, { useState, useEffect, useContext } from "react"
import { connect } from "react-redux"
import { Table, Row, Col } from "react-bootstrap"

import Countdown from "pages/landing/components/countdown"
import ScreenContext from "shared/context/screenContext"

import "styles/landing.css"

const Leaderboard = (props: {
    waitlist: any[]
    user: { email: any; ranking: number }
}) => {
    const { width } = useContext(ScreenContext)
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
        if (width > 720) {
            return (
                <tr className={userRow ? "userRow" : ""}>
                    <td className="rankingColumn">
                        <div className={idx <= 3 ? "topThree" : "bottomThree"}>
                            {idx}
                        </div>
                    </td>
                    <td className="nameColumn">
                        <div
                            style={{
                                position: "relative",
                                top: idx <= 3 ? 7 : 0,
                            }}
                        >
                            {name}
                        </div>
                    </td>
                    <td>
                        <div
                            style={{
                                position: "relative",
                                top: idx <= 3 ? 7 : 0,
                            }}
                        >
                            {referrals}
                        </div>
                    </td>
                    <td className="pointsColumn">
                        <div
                            style={{
                                position: "relative",
                                top: idx <= 3 ? 7 : 0,
                            }}
                        >
                            {points}
                        </div>
                    </td>
                </tr>
            )
        } else {
            const barWidth =
                Math.round(
                    Math.max((points * 100) / props.waitlist[0].points, 5)
                ) + "%"

            return (
                <div
                    style={{
                        color: "white",
                        background: userRow ? "#5b47d4" : "rgba(91,71,212,0.4)",
                        borderRadius: 5,
                        padding: "20px 25px",
                        marginBottom: 15,
                    }}
                >
                    <Row style={{ width: "100%" }}>
                        <Col xs={2}>
                            <div
                                style={{
                                    fontSize: idx <= 3 ? 25 : 16,
                                    fontWeight: "bold",
                                    color: "white",
                                }}
                            >
                                {idx}
                            </div>
                        </Col>
                        <Col
                            xs={6}
                            style={{ fontWeight: userRow ? "bold" : "normal" }}
                        >
                            {name}
                        </Col>
                        <Col xs={4} style={{ paddingRight: 0 }}>
                            <div
                                style={{
                                    fontSize: 16,
                                    textAlign: "right",
                                }}
                            >
                                <strong>{points}</strong> pts
                            </div>
                        </Col>
                    </Row>
                    <Row>
                        <Col xs={2}></Col>
                        <Col xs={10} style={{ paddingLeft: 10 }}>
                            <div
                                style={{
                                    height: 4,
                                    borderRadius: 2,
                                    width: barWidth,
                                    background:
                                        "linear-gradient(90.15deg, #93F1D9 0%, #6AEEF0 100%)",
                                }}
                            ></div>
                        </Col>
                    </Row>
                </div>
            )
        }
    }

    // Renders correct leaderboard rows based on whether user is on waitlist and their ranking
    const getRows = () => {
        const topThree = topSix.slice(0, 3)
        if (!props.waitlist) {
            return <tr>No data to show.</tr>
        } else if (!props.user.email || props.user.ranking <= 5) {
            const bottomThree = topSix.slice(3, 6)
            return topThree
                .map((user: any, idx: number) => {
                    return renderRow(
                        idx + 1,
                        idx + 1 === props.user.ranking,
                        user.name,
                        user.referrals,
                        user.points
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return renderRow(
                            idx + 4,
                            idx + 4 === props.user.ranking,
                            user.name,
                            user.referrals,
                            user.points
                        )
                    })
                )
        } else if (props.user.ranking === props.waitlist.length) {
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
                            props.user.ranking - 2 + idx,
                            idx === 2,
                            user.name,
                            user.referrals,
                            user.points
                        )
                    })
                )
        } else {
            const bottomThree = props.waitlist.slice(
                props.user.ranking - 2,
                props.user.ranking + 1
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
                            props.user.ranking - 1 + idx,
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
                    display: width > 720 ? "flex" : "block",
                    justifyContent: "space-between",
                }}
            >
                <h1
                    style={{
                        fontWeight: "bold",
                        color: "white",
                        marginBottom: "30px",
                        fontSize: "40px",
                    }}
                >
                    Leaderboard
                </h1>
                <div
                    style={{ position: "relative", top: width > 720 ? 25 : 0 }}
                >
                    <Countdown type="small" />
                </div>
            </div>
            <div
                style={{
                    overflowY: width > 720 ? "scroll" : "auto",
                    maxHeight: width > 720 ? 550 : "none",
                    width: "100%",
                    borderRadius: 5,
                    boxShadow: "0px 4px 30px rgba(0, 0, 0, 0.1)",
                    zIndex: 2,
                    paddingTop: width > 720 ? 0 : 20,
                }}
            >
                {width > 720 ? (
                    <Table>
                        <thead>
                            <tr>
                                <th style={{ paddingLeft: 50 }}></th>
                                <th>Name</th>
                                <th>Referrals</th>
                                <th>Points</th>
                            </tr>
                        </thead>
                        <tbody>{getRows()}</tbody>
                    </Table>
                ) : (
                    <div style={{ marginBottom: 30, marginTop: 20 }}>
                        {getRows()}
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    MainReducer: { user: any; waitlist: any[] }
}) => {
    return {
        user: state.MainReducer.user,
        waitlist: state.MainReducer.waitlist,
    }
}

export default connect(mapStateToProps)(Leaderboard)
