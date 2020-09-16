import React, { useState, useEffect } from "react"
import { connect } from "react-redux"
import { Table } from "react-bootstrap"

import "styles/landing.css"

const LEADERS_CUTOFF = 20

const Leaderboard = (props: {
    waitlist: any[]
    user: { email: any; ranking: number }
}) => {
    const [topSix, setTopSix]: any[] = useState([])

    useEffect(() => {
        setTopSix(props.waitlist ? props.waitlist.slice(0, LEADERS_CUTOFF) : [])
    }, [props.waitlist])

    const getRows = () => {
        const topThree = topSix.slice(0, 10)
        if (!props.waitlist) {
            return <tr>No data to show.</tr>
        } else if (!props.user.email || props.user.ranking < LEADERS_CUTOFF) {
            const bottomThree = topSix.slice(LEADERS_CUTOFF / 2, LEADERS_CUTOFF)
            return topThree
                .map((user: any, idx: number) => {
                    return (
                        <tr
                            className={
                                idx + 1 === props.user.ranking ? "userRow" : ""
                            }
                        >
                            <td className="rankingColumn">
                                <div className="topThree">{idx + 1}</div>
                            </td>
                            <td className="nameColumn">{user.name}</td>
                            <td>{user.referrals}</td>
                            <td className="pointsColumn">{user.points}</td>
                        </tr>
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return (
                            <tr
                                className={
                                    idx + 4 === props.user.ranking
                                        ? "userRow"
                                        : ""
                                }
                            >
                                <td className="rankingColumn">
                                    {idx + LEADERS_CUTOFF + 1}
                                </td>
                                <td className="nameColumn">{user.name}</td>
                                <td>{user.referrals}</td>
                                <td className="pointsColumn">{user.points}</td>
                            </tr>
                        )
                    })
                )
        } else if (props.user.ranking === props.waitlist.length) {
            const bottomThree = props.waitlist.slice(-10)
            return topThree
                .map((user: any, idx: number) => {
                    return (
                        <tr>
                            <td className="rankingColumn">
                                <div className="topThree">{idx + 1}</div>
                            </td>
                            <td className="nameColumn">{user.name}</td>
                            <td>{user.referrals}</td>
                            <td className="pointsColumn">{user.points}</td>
                        </tr>
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return (
                            <tr className={idx === 9 ? "userRow" : ""}>
                                <td className="rankingColumn">
                                    {props.user.ranking - 2 + idx}
                                </td>
                                <td className="nameColumn">{user.name}</td>
                                <td>{user.referrals}</td>
                                <td className="pointsColumn">{user.points}</td>
                            </tr>
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
                    return (
                        <tr>
                            <td className="rankingColumn">
                                <div className="topThree">{idx + 1}</div>
                            </td>
                            <td className="nameColumn">{user.name}</td>
                            <td>{user.referrals}</td>
                            <td className="pointsColumn">{user.points}</td>
                        </tr>
                    )
                })
                .concat(
                    bottomThree.map((user: any, idx: number) => {
                        return (
                            <tr className={idx === 1 ? "userRow" : ""}>
                                <td className="rankingColumn">
                                    {props.user.ranking - 1 + idx}
                                </td>
                                <td className="nameColumn">{user.name}</td>
                                <td>{user.referrals}</td>
                                <td className="pointsColumn">{user.points}</td>
                            </tr>
                        )
                    })
                )
        }
    }
    return (
        <div className="leaderboard-container">
            <h1
                style={{
                    fontWeight: "bold",
                    color: "#111111",
                    marginBottom: "30px",
                    fontSize: "40px",
                }}
            >
                Leaderboard
            </h1>
            <div
                style={{
                    overflowY: "scroll",
                    maxHeight: 750,
                    width: "100%",
                    borderRadius: 10,
                    boxShadow: "0px 4px 30px rgba(0, 0, 0, 0.1)",
                }}
            >
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
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: any; waitlist: any[] }
}) => {
    return {
        user: state.AuthReducer.user,
        waitlist: state.AuthReducer.waitlist,
    }
}

export default connect(mapStateToProps)(Leaderboard)
