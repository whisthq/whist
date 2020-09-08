import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { Table } from "react-bootstrap";

import "styles/landing.css";

const Leaderboard = (props) => {
    // const [onWaitlist, setOnWaitlist] = useState(false);
    const [topSix, setTopSix] = useState([]);

    useEffect(() => {
        console.log(props.waitlist);
        console.log(props.user);
        // setOnWaitlist(props.user && props.email);
        setTopSix(props.waitlist ? props.waitlist.slice(0, 6) : []);
    }, [props]);

    const getRows = () => {
        if (!props.user.email || props.user.ranking <= 5) {
            return topSix.map((user, idx) => {
                return (
                    <tr
                        className={
                            idx + 1 == props.user.ranking ? "userRow" : ""
                        }
                    >
                        <td>{idx + 1}</td>
                        <td>{user.name}</td>
                        <td>{user.referrals}</td>
                        <td>{user.points}</td>
                    </tr>
                );
            });
        } else if (props.user.ranking == props.waitlist.length) {
            const topThree = topSix.slice(0, 3);
            const bottomThree = props.waitlist.slice(-3);
            return topThree
                .map((user, idx) => {
                    return (
                        <tr>
                            <td>{idx + 1}</td>
                            <td>{user.name}</td>
                            <td>{user.referrals}</td>
                            <td>{user.points}</td>
                        </tr>
                    );
                })
                .concat(
                    bottomThree.map((user, idx) => {
                        return (
                            <tr className={idx == 2 ? "userRow" : ""}>
                                <td>{props.user.ranking - 2 + idx}</td>
                                <td>{user.name}</td>
                                <td>{user.referrals}</td>
                                <td>{user.points}</td>
                            </tr>
                        );
                    })
                );
        } else {
            const topThree = topSix.slice(0, 3);
            const neighbors = props.waitlist.slice(
                props.user.ranking - 2,
                props.user.ranking + 1
            );
            return topThree
                .map((user, idx) => {
                    return (
                        <tr>
                            <td>{idx + 1}</td>
                            <td>{user.name}</td>
                            <td>{user.referrals}</td>
                            <td>{user.points}</td>
                        </tr>
                    );
                })
                .concat(
                    neighbors.map((user, idx) => {
                        return (
                            <tr className={idx == 1 ? "userRow" : ""}>
                                <td>{props.user.ranking - 1 + idx}</td>
                                <td>{user.name}</td>
                                <td>{user.referrals}</td>
                                <td>{user.points}</td>
                            </tr>
                        );
                    })
                );
        }
    };
    return (
        <div style={{ marginTop: "200px", marginBottom: "200px" }}>
            <Table bordered>
                <thead>
                    <tr>
                        <th>Ranking</th>
                        <th>Name</th>
                        <th>Referrals</th>
                        <th>Points</th>
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
