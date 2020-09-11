import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { Table } from "react-bootstrap";

import "styles/landing.css";

const Leaderboard = (props) => {
    const [topSix, setTopSix] = useState([]);

    useEffect(() => {
        console.log("use effect leaderboard");
        console.log(props.waitlist);
        setTopSix(props.waitlist ? props.waitlist.slice(0, 6) : []);
    }, [props.waitlist]);

    const getRows = () => {
        const topThree = topSix.slice(0, 3);
        if (!props.waitlist) {
            return <tr>No data to show.</tr>;
        } else if (!props.user.email || props.user.ranking <= 5) {
            const bottomThree = topSix.slice(-3);
            return topThree
                .map((user, idx) => {
                    return (
                        <tr
                            className={
                                idx + 1 == props.user.ranking ? "userRow" : ""
                            }
                        >
                            <td className="rankingColumn">
                                <div className="topThree">{idx + 1}</div>
                            </td>
                            <td className="nameColumn">{user.name}</td>
                            <td>{user.referrals}</td>
                            <td className="pointsColumn">{user.points}</td>
                        </tr>
                    );
                })
                .concat(
                    bottomThree.map((user, idx) => {
                        return (
                            <tr
                                className={
                                    idx + 4 == props.user.ranking
                                        ? "userRow"
                                        : ""
                                }
                            >
                                <td className="rankingColumn">{idx + 4}</td>
                                <td className="nameColumn">{user.name}</td>
                                <td>{user.referrals}</td>
                                <td className="pointsColumn">{user.points}</td>
                            </tr>
                        );
                    })
                );
        } else if (props.user.ranking == props.waitlist.length) {
            const bottomThree = props.waitlist.slice(-3);
            return topThree
                .map((user, idx) => {
                    return (
                        <tr>
                            <td className="rankingColumn">
                                <div className="topThree">{idx + 1}</div>
                            </td>
                            <td className="nameColumn">{user.name}</td>
                            <td>{user.referrals}</td>
                            <td className="pointsColumn">{user.points}</td>
                        </tr>
                    );
                })
                .concat(
                    bottomThree.map((user, idx) => {
                        return (
                            <tr className={idx == 2 ? "userRow" : ""}>
                                <td className="rankingColumn">
                                    {props.user.ranking - 2 + idx}
                                </td>
                                <td className="nameColumn">{user.name}</td>
                                <td>{user.referrals}</td>
                                <td className="pointsColumn">{user.points}</td>
                            </tr>
                        );
                    })
                );
        } else {
            const neighbors = props.waitlist.slice(
                props.user.ranking - 2,
                props.user.ranking + 1
            );
            return topThree
                .map((user, idx) => {
                    return (
                        <tr>
                            <td className="rankingColumn">
                                <div className="topThree">{idx + 1}</div>
                            </td>
                            <td className="nameColumn">{user.name}</td>
                            <td>{user.referrals}</td>
                            <td className="pointsColumn">{user.points}</td>
                        </tr>
                    );
                })
                .concat(
                    neighbors.map((user, idx) => {
                        return (
                            <tr className={idx == 1 ? "userRow" : ""}>
                                <td className="rankingColumn">
                                    {props.user.ranking - 1 + idx}
                                </td>
                                <td className="nameColumn">{user.name}</td>
                                <td>{user.referrals}</td>
                                <td className="pointsColumn">{user.points}</td>
                            </tr>
                        );
                    })
                );
        }
    };
    return (
        <div
            style={{
                display: "flex",
                flexDirection: "column",
                alignItems: "flex-start",
                marginTop: "9vh",
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
            <Table>
                <thead>
                    <tr>
                        <th className="rankingColumn">Ranking</th>
                        <th>Name</th>
                        <th>Referrals</th>
                        <th className="pointsColumn">Points</th>
                    </tr>
                </thead>
                <tbody>{getRows()}</tbody>
            </Table>
        </div>
    );
};

const mapStateToProps = (state) => {
    return {
        user: state.AuthReducer.user,
        waitlist: state.AuthReducer.waitlist,
    };
};

export default connect(mapStateToProps)(Leaderboard);
