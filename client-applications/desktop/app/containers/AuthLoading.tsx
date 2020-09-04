import React, { Component } from "react";
import Titlebar from "react-electron-titlebar";

import { connect } from "react-redux";
import { history } from "store/configureStore";
import { loginUser, setOS, googleLogin } from "actions/counter";

import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons";
import Background from "resources/images/background.jpg";

class AuthLoading extends Component {
    constructor(props) {
        super(props);
        this.state = {
            live: true,
        };
    }

    componentDidMount() {
        const storage = require("electron-json-storage");
        const os = require("os");
        this.props.dispatch(setOS(os.platform()));

        if (this.state.live) {
            if (this.props.username && this.props.public_ip) {
                history.push("/dashboard");
            }

            let component = this;
            storage.get("credentials", function (error, data) {
                if (error) throw error;

                if (data && Object.keys(data).length > 0) {
                    if (
                        data.username != "" &&
                        data.password != "" &&
                        component.state.live
                    ) {
                        component.props.dispatch(
                            loginUser(data.username, data.password)
                        );
                    } else {
                        history.push("/login");
                    }
                } else {
                    history.push("/login");
                }
            });
        }
    }

    render() {
        return (
            <div
                style={{
                    backgroundImage: `url(${Background})`,
                    position: "absolute",
                    width: "100vw",
                    height: "100vh",
                    backgroundSize: "cover",
                    backgroundPosition: "center",
                    backgroundRepeat: "no-repeat",
                }}
            >
                {this.props.os === "win32" ? (
                    <Titlebar backgroundColor="#000000" />
                ) : (
                    <div />
                )}

                {this.state.live ? (
                    <div
                        style={{
                            height: "100vh",
                            display: "flex",
                            flexDirection: "column",
                            justifyContent: "center",
                            alignItems: "center",
                        }}
                    >
                        <FontAwesomeIcon
                            icon={faCircleNotch}
                            spin
                            style={{ color: "white", height: "30" }}
                        />

                        <div>Loading</div>
                    </div>
                ) : (
                    <div
                        style={{
                            lineHeight: 1.5,
                            margin: "150px auto",
                            maxWidth: 400,
                        }}
                    >
                        {" "}
                        We are currently pushing out a critical Linux update.
                        Your app will be back online very soon. We apologize for
                        the inconvenience!
                    </div>
                )}
            </div>
        );
    }
}

function mapStateToProps(state) {
    return {
        username: state.counter.username,
        public_ip: state.counter.public_ip,
        warning: state.counter.warning,
        os: state.counter.os,
    };
}

export default connect(mapStateToProps)(AuthLoading);
