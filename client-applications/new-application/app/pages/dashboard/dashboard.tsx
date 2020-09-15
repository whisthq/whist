import React, { Component } from "react";
import { connect } from "react-redux";

class Dashboard extends Component {
    render() {
        return (
            <div
                style={{
                    marginTop: 200,
                    textAlign: "center",
                }}
            >
                New dashboard here!
            </div>
        );
    }
}

export default connect()(Dashboard);
