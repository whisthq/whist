import React, { Component } from "react";
import styles from "pages/PageLogin/Login.css";
import Titlebar from "react-electron-titlebar";
import Logo from "resources/images/logo.svg";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons";

class UpdateScreen extends Component {
    constructor(props) {
        super(props);
        this.state = {
            updateScreen: false,
            percentageLeft: 300,
            percentageDownloaded: 0,
            downloadSpeed: 0,
            transferred: 0,
            total: 0,
            downloadError: "",
        };
    }

    componentDidMount() {
        const ipc = require("electron").ipcRenderer;
        let component = this;

        ipc.on("update", (event, update) => {
            if (update) {
                component.setState({ updateScreen: true });
            }
        });

        ipc.on("percent", (event, percent) => {
            percent = percent * 3;
            this.setState({
                percentageLeft: 300 - percent,
                percentageDownloaded: percent,
            });
        });

        ipc.on("download-speed", (event, speed) => {
            this.setState({ downloadSpeed: (speed / 1000000).toFixed(2) });
        });

        ipc.on("transferred", (event, transferred) => {
            this.setState({ transferred: (transferred / 1000000).toFixed(2) });
        });

        ipc.on("total", (event, total) => {
            this.setState({ total: (total / 1000000).toFixed(2) });
        });

        ipc.on("error", (event, error) => {
            this.setState({ downloadError: error });
        });

        ipc.on("downloaded", (event, downloaded) => {});
    }

    render() {
        return (
            <div>
                {this.state.updateScreen ? (
                    <div
                        style={{
                            position: "fixed",
                            top: 0,
                            left: 0,
                            width: 1000,
                            height: 680,
                            backgroundColor: "#0B172B",
                            zIndex: 1000,
                        }}
                    >
                        {this.props.os === "win32" ? (
                            <div>
                                <Titlebar backgroundColor="#000000" />
                            </div>
                        ) : (
                            <div style={{ marginTop: 10 }}></div>
                        )}
                        <div className={styles.landingHeader}>
                            <div className={styles.landingHeaderLeft}>
                                <img src={Logo} width="20" height="20" />
                                <span className={styles.logoTitle}>
                                    Fractal
                                </span>
                            </div>
                            <div className={styles.landingHeaderRight}>
                                <span
                                    id="forgotButton"
                                    onClick={this.ForgotPassword}
                                >
                                    Forgot Password?
                                </span>
                                <button
                                    type="button"
                                    className={styles.signupButton}
                                    id="signup-button"
                                    onClick={this.SignUp}
                                >
                                    Sign Up
                                </button>
                            </div>
                        </div>
                        <div style={{ position: "relative" }}>
                            <div
                                style={{
                                    paddingTop: 180,
                                    textAlign: "center",
                                    color: "white",
                                    width: 900,
                                }}
                            >
                                <div
                                    style={{
                                        display: "flex",
                                        alignItems: "center",
                                        justifyContent: "center",
                                    }}
                                >
                                    <div
                                        style={{
                                            width: `${this.state.percentageDownloaded}px`,
                                            height: 6,
                                            background:
                                                "linear-gradient(258.54deg, #5ec3eb 0%, #d023eb 100%)",
                                        }}
                                    ></div>
                                    <div
                                        style={{
                                            width: `${this.state.percentageLeft}px`,
                                            height: 6,
                                            background: "#111111",
                                        }}
                                    ></div>
                                </div>
                                {this.state.downloadError === "" ? (
                                    <div
                                        style={{
                                            marginTop: 10,
                                            fontSize: 14,
                                            display: "flex",
                                            alignItems: "center",
                                            justifyContent: "center",
                                        }}
                                    >
                                        <div style={{ color: "#D6D6D6" }}>
                                            <FontAwesomeIcon
                                                icon={faCircleNotch}
                                                spin
                                                style={{
                                                    color: "#5EC4EB",
                                                    marginRight: 4,
                                                    width: 12,
                                                }}
                                            />{" "}
                                            Downloading Update (
                                            {this.state.downloadSpeed} Mbps)
                                        </div>
                                    </div>
                                ) : (
                                    <div
                                        style={{
                                            marginTop: 10,
                                            fontSize: 14,
                                            display: "flex",
                                            alignItems: "center",
                                            justifyContent: "center",
                                        }}
                                    >
                                        <div style={{ color: "#D6D6D6" }}>
                                            {this.state.downloadError}
                                        </div>
                                    </div>
                                )}
                                <div
                                    style={{
                                        color: "#C9C9C9",
                                        fontSize: 10,
                                        margin: "auto",
                                        marginTop: 5,
                                    }}
                                >
                                    {this.state.transferred} /{" "}
                                    {this.state.total} MB Downloaded
                                </div>
                            </div>
                        </div>
                    </div>
                ) : (
                    <div></div>
                )}
            </div>
        );
    }
}

export default UpdateScreen;
