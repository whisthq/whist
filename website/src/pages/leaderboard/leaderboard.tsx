import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { Table } from "react-bootstrap";

import "styles/landing.css";

const Leaderboard = (props) => {
    // const [onWaitlist, setOnWaitlist] = useState(false);
    const [users, setUsers] = useState([]);

    useEffect(() => {
        console.log(props.waitlist);
        // setOnWaitlist(props.user && props.email);
        setUsers(props.waitlist ? props.waitlist.slice(0, 6) : []);
    }, [props]);

    const getRows = () => {
        return users.map((user, idx) => {
            return (
                <tr>
                    <td>{idx + 1}</td>
                    <td>{user.name}</td>
                    <td>{user.referrals}</td>
                    <td>{user.points}</td>
                </tr>
            );
        });
    };
    return (
        <div style={{ height: "100vh" }}>
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
        email: state.AuthReducer.user.email,
        name: state.AuthReducer.user.name,
        waitlist: state.AuthReducer.waitlist,
    };
};

export default connect(mapStateToProps)(Leaderboard);
