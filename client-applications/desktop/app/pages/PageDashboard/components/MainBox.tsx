import React, { Component } from "react";
import { connect } from "react-redux";
import Popup from "reactjs-popup";
import ToggleButton from "react-toggle-button";
import Slider from "react-input-slider";

import styles from "pages/PageDashboard/Dashboard.css";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import {
    faCheck,
    faCogs,
    faKeyboard,
    faInfoCircle,
    faPencilAlt,
    faPlus,
    faCircleNotch,
    faWrench,
    faUpload,
} from "@fortawesome/free-solid-svg-icons";

import Window from "resources/images/window.svg";
import Speedometer from "resources/images/speedometer.svg";
import Mountain from "resources/images/mountain.jpg";
import Scale from "resources/images/scale.svg";

import {
    askFeedback,
    changeWindow,
    attachDisk,
    restartPC,
    sendLogs,
    changeStatusMessage,
    readyToConnect,
} from "actions/counter";

class MainBox extends Component {
    constructor(props) {
        super(props);
        this.state = {
            launches: 0,
            windowMode: false,
            mbps: 500,
            nickname: "",
            editNickname: -1,
            diskAttaching: false,
            launched: false,
            reattached: false,
            restartPopup: false,
            vmRestarting: false,
        };
    }

    ExitSettings = () => {
        this.props.dispatch(changeWindow(this.props.default));
    };

    ChangeNickname = (evt: any) => {
        this.setState({
            nickname: evt.target.value,
        });
    };

    NicknameEdit = (index: any) => {
        this.setState({ editNickname: index });
    };

    openRestartPopup = (open: any) => {
        this.setState({ restartPopup: open });
    };

    SendLogs = () => {
        var fs = require("fs");
        var appRootDir = require("electron").remote.app.getAppPath();
        const os = require("os");
        let component = this;

        if (os.platform() === "darwin" || os.platform() === "linux") {
            // mac & linux
            // get logs and connection_id from Fractal MacOS/Linux Ubuntu caches
            // cache is located in /users/USERNAME/.fractal or in /home/USERNAME/.fractal
            var connection_id = parseInt(
                fs
                    .readFileSync(
                        process.env.HOME + "/.fractal/connection_id.txt"
                    )
                    .toString()
            );

            fs.readFile(
                process.env.HOME + "/.fractal/log.txt",
                "utf8",
                function (err, data) {
                    if (err) {
                        console.log(err);
                    } else {
                        component.props.dispatch(sendLogs(connection_id, data));
                    }
                }
            );
            // console.log(logs)
        } else if (os.platform() === "win32") {
            // windows
            // get logs from the executable directory, no cache on Windows
            var logs = fs
                .readFileSync(process.cwd() + "\\protocol\\desktop\\log.txt")
                .toString();
            var connection_id = parseInt(
                fs
                    .readFileSync(
                        process.cwd() + "\\protocol\\desktop\\connection_id.txt"
                    )
                    .toString()
            );
            this.props.dispatch(sendLogs(connection_id, logs));
        }
    };

    // Launches the protocol
    LaunchProtocol = () => {
        if (
            this.state.reattached &&
            this.props.public_ip &&
            this.props.public_ip != ""
        ) {
            this.setState({ launched: true });
            this.props.dispatch(
                changeStatusMessage(
                    "Boot request sent to server. Waiting for a response."
                )
            );
            if (this.state.launches == 0) {
                this.setState({ launches: 1, reattached: false }, function () {
                    var child = require("child_process").spawn;
                    var appRootDir = require("electron").remote.app.getAppPath();
                    const os = require("os");

                    // check which OS we're on to properly launch the protocol
                    if (os.platform() === "darwin") {
                        // mac
                        // path when electron app is packaged as .dmg
                        var path = appRootDir + "/protocol-build/";
                        path = path.replace("/Resources/app.asar", "");
                        path = path.replace("/desktop/app", "/desktop");
                        var executable = "./FractalClient";
                    } else if (os.platform() === "linux") {
                        var path = process.cwd() + "/protocol-build";
                        path = path.replace("/release", "");
                        var executable = "./FractalClient";
                    } else if (os.platform() === "win32") {
                        // windows
                        // path when electron app is packaged as .nsis (to use as working directory)
                        var path = process.cwd() + "\\protocol\\desktop";
                        var executable = "FractalClient.exe";
                    }

                    // 0 dimensions is full-screen in protocol, windowed-mode starts at half screen
                    // and can be resized in the protocol directly. DPI calculation done in protocol
                    var screenWidth = this.state.windowMode
                        ? window.screen.width / 2
                        : 0;
                    var screenHeight = this.state.windowMode
                        ? window.screen.height / 2
                        : 0;

                    var parameters = [
                        this.props.public_ip,
                        screenWidth,
                        screenHeight,
                        this.state.mbps,
                    ];

                    // Starts the protocol
                    const protocol = child(executable, parameters, {
                        cwd: path,
                        detached: true,
                        stdio: "ignore",
                    });
                    //Listener for closing the stream window
                    protocol.on("close", (code) => {
                        this.SendLogs();
                        this.setState({
                            launches: 0,
                            launched: false,
                            reattached: false,
                            diskAttaching: false,
                        });
                        this.props.dispatch(askFeedback(true));
                    });
                });
            }
        } else {
            this.setState({ launched: false, diskAttaching: true });
            this.props.dispatch(attachDisk());
        }
    };

    toggleWindow = (mode: any) => {
        const storage = require("electron-json-storage");
        let component = this;

        this.setState({ windowMode: !mode }, function () {
            storage.set("window", { windowMode: !mode });
        });
    };

    changeMbps = (mbps) => {
        const storage = require("electron-json-storage");

        this.setState({ mbps: parseFloat(mbps.toFixed(2)) });
        if (mbps === 50) {
            // set to unlimited, aka 500, is a user chooses Unlimited
            mbps = 500;
        }
        storage.set("settings", { mbps: mbps });
    };

    changeScale = (scale) => {
        const storage = require("electron-json-storage");
        let component = this;

        this.setState({ scale: scale }, function () {
            storage.set("scale", { scale: scale });
        });
    };

    OpenDashboard = () => {
        const { shell } = require("electron");
        shell.openExternal("https://www.fractalcomputers.com/dashboard");
    };

    RestartPC = () => {
        this.setState(
            { restartPopup: false, vmRestarting: true },
            function () {}
        );
        this.props.dispatch(restartPC());
    };

    OpenSupport = () => {
        this.props.dispatch(askFeedback(true));
        this.setState({ restartPopup: false });
    };

    componentDidMount() {
        const storage = require("electron-json-storage");
        let component = this;

        storage.get("settings", function (error, data) {
            if (error) throw error;
            if (data.mbps) {
                component.setState({ mbps: data.mbps });
            }
        });

        storage.get("window", function (error, data) {
            if (error) throw error;
            component.setState({ windowMode: data.windowMode });
        });

        this.props.dispatch(askFeedback(false));
    }

    componentDidUpdate(prevProps) {
        if (
            prevProps.attach_attempts != this.props.attach_attempts &&
            this.state.diskAttaching
        ) {
            this.setState({ diskAttaching: false, reattached: true });
        }

        if (
            prevProps.restart_attempts != this.props.restart_attempts &&
            this.state.vmRestarting
        ) {
            this.setState({ vmRestarting: false });
        }
        if (
            !prevProps.ready_to_connect &&
            this.props.ready_to_connect &&
            !this.state.launched
        ) {
            let component = this;
            this.setState(
                { diskAttaching: false, reattached: true, launched: true },
                function () {
                    this.props.dispatch(readyToConnect(false));
                    this.LaunchProtocol();
                }
            );
        }
    }

    render() {
        const BigButton = () => {
            if (this.props.account_locked && this.props.isUser) {
                return (
                    <div className={styles.pointerOnHover}>
                        <div
                            className={styles.bigButton}
                            onClick={this.OpenDashboard}
                        >
                            <div className={styles.centerContent}>
                                <div className={styles.centerText}>
                                    Oops! Your Free Trial Has Expired
                                </div>
                                <div className={styles.centerSubtext}>
                                    Please provide your payment details in order
                                    to access your cloud PC.
                                </div>
                            </div>
                        </div>
                    </div>
                );
            }

            if (this.state.vmRestarting) {
                return (
                    <div className={styles.bigButton}>
                        <div className={styles.centerContent}>
                            <div>
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{ color: "#111111", height: 30 }}
                                />
                            </div>
                            <div className={styles.centerText}>Restarting</div>
                        </div>
                    </div>
                );
            }

            if (this.props.fetchStatus) {
                if (this.props.disk !== "") {
                    if (!this.state.diskAttaching && this.state.launched) {
                        return (
                            <div className={styles.bigButton}>
                                <div className={styles.centerContent}>
                                    <div>
                                        <FontAwesomeIcon
                                            icon={faCircleNotch}
                                            spin
                                            style={{
                                                color: "#111111",
                                                height: 30,
                                            }}
                                        />
                                    </div>
                                    <div className={styles.centerText}>
                                        Streaming
                                    </div>
                                </div>
                            </div>
                        );
                    } else if (
                        !this.state.diskAttaching &&
                        !this.state.launched
                    ) {
                        return (
                            <div className={styles.pointerOnHover}>
                                <div
                                    onClick={this.LaunchProtocol}
                                    className={styles.bigButton}
                                    style={{
                                        backgroundImage:
                                            "linear-gradient(to bottom, rgba(255, 255, 255, 0.1), rgba(255, 255, 255, 0.1)), url(" +
                                            Mountain +
                                            ")",
                                    }}
                                >
                                    <div
                                        style={{
                                            color: "white",
                                            position: "absolute",
                                            bottom: 15,
                                            left: 15,
                                            fontWeight: "bold",
                                        }}
                                    >
                                        Launch My Cloud PC
                                    </div>
                                </div>
                            </div>
                        );
                    } else if (this.state.diskAttaching) {
                        return (
                            <div className={styles.bigButton}>
                                <div className={styles.centerContent}>
                                    <div>
                                        <FontAwesomeIcon
                                            icon={faCircleNotch}
                                            spin
                                            style={{
                                                color: "#111111",
                                                height: 30,
                                            }}
                                        />
                                    </div>
                                    <div className={styles.centerText}>
                                        Booting your cloud PC
                                    </div>
                                    <div className={styles.centerSubtext}>
                                        {this.props.status_message}
                                    </div>
                                </div>
                            </div>
                        );
                    }
                } else {
                    return (
                        <div className={styles.pointerOnHover}>
                            <div
                                onClick={this.OpenDashboard}
                                className={styles.bigButton}
                            >
                                <div className={styles.centerContent}>
                                    <div>
                                        <FontAwesomeIcon
                                            icon={faPlus}
                                            style={{
                                                height: 30,
                                                color: "#111111",
                                            }}
                                        />
                                    </div>
                                    <div
                                        style={{
                                            marginTop: 25,
                                            color: "#111111",
                                        }}
                                        className={styles.centerText}
                                    >
                                        <span className={styles.blueGradient}>
                                            Create My Cloud PC
                                        </span>
                                    </div>
                                    <div className={styles.centerSubtext}>
                                        Transform your computer into a
                                        GPU-powered workstation.
                                    </div>
                                </div>
                            </div>
                        </div>
                    );
                }
            } else {
                return (
                    <div className={styles.bigButton}>
                        <div className={styles.centerContent}>
                            <div>
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{ color: "#111111", height: 30 }}
                                />
                            </div>
                            <div className={styles.centerText}>
                                Loading your account
                            </div>
                        </div>
                    </div>
                );
            }
        };

        if (this.props.currentWindow === "main") {
            return (
                <div>
                    {BigButton()}
                    <div style={{ display: "flex", marginTop: 20 }}>
                        <div
                            style={{
                                width: "50%",
                                paddingRight: 20,
                                textAlign: "center",
                            }}
                        >
                            {this.props.public_ip &&
                            this.props.public_ip !== "" ? (
                                <Popup
                                    open={this.state.restartPopup}
                                    trigger={
                                        <div
                                            className={styles.bigBox}
                                            onClick={() =>
                                                this.openRestartPopup(true)
                                            }
                                            style={{
                                                background: "white",
                                                borderRadius: 5,
                                                padding: 10,
                                                minHeight: 90,
                                                paddingTop: 20,
                                                paddingBottom: 0,
                                            }}
                                        >
                                            <FontAwesomeIcon
                                                icon={faWrench}
                                                style={{
                                                    height: 40,
                                                    color: "#111111",
                                                }}
                                            />
                                            <div
                                                style={{
                                                    marginTop: 5,
                                                    fontSize: 14,
                                                    fontWeight: "bold",
                                                    color: "#111111",
                                                }}
                                            >
                                                Troubleshoot
                                            </div>
                                        </div>
                                    }
                                    modal
                                    contentStyle={{
                                        width: 350,
                                        color: "#111111",
                                        borderRadius: 5,
                                        backgroundColor: "white",
                                        border: "none",
                                        height: 170,
                                        padding: 30,
                                    }}
                                >
                                    <div
                                        style={{
                                            fontWeight: "bold",
                                            fontSize: 20,
                                        }}
                                    >
                                        Have Trouble Connecting?
                                    </div>
                                    <div
                                        style={{
                                            fontSize: 14,
                                            lineHeight: 1.4,
                                            width: 300,
                                            margin: "20px auto",
                                        }}
                                    >
                                        Restarting your cloud PC could help.
                                        Note that restarting will close any
                                        files or applications you have open.
                                    </div>
                                    {!this.state.vmRestarting ? (
                                        <button
                                            onClick={this.RestartPC}
                                            type="button"
                                            className={styles.signupButton}
                                            id="signup-button"
                                            style={{
                                                width: 300,
                                                marginLeft: 0,
                                                marginTop: 20,
                                                fontFamily: "Maven Pro",
                                                fontWeight: "bold",
                                            }}
                                        >
                                            Restart Cloud PC
                                        </button>
                                    ) : (
                                        <button
                                            type="button"
                                            className={styles.signupButton}
                                            id="signup-button"
                                            style={{
                                                width: 300,
                                                marginLeft: 0,
                                                marginTop: 20,
                                                fontFamily: "Maven Pro",
                                                fontWeight: "bold",
                                            }}
                                        >
                                            <FontAwesomeIcon
                                                icon={faCircleNotch}
                                                spin
                                                style={{
                                                    color: "#1ba8e0",
                                                    height: 12,
                                                }}
                                            />
                                        </button>
                                    )}
                                </Popup>
                            ) : (
                                <Popup
                                    trigger={
                                        <div
                                            className={styles.bigBox}
                                            style={{
                                                background: "white",
                                                borderRadius: 5,
                                                padding: 10,
                                                minHeight: 90,
                                                paddingTop: 20,
                                                paddingBottom: 0,
                                            }}
                                        >
                                            <FontAwesomeIcon
                                                icon={faWrench}
                                                style={{
                                                    height: 40,
                                                    color: "#111111",
                                                }}
                                            />
                                            <div
                                                style={{
                                                    marginTop: 5,
                                                    fontSize: 14,
                                                    fontWeight: "bold",
                                                    color: "#111111",
                                                }}
                                            >
                                                Troubleshoot
                                            </div>
                                        </div>
                                    }
                                    modal
                                    contentStyle={{
                                        width: 350,
                                        color: "#111111",
                                        borderRadius: 5,
                                        backgroundColor: "white",
                                        border: "none",
                                        height: 125,
                                        padding: 30,
                                    }}
                                >
                                    <div
                                        style={{
                                            fontWeight: "bold",
                                            fontSize: 20,
                                        }}
                                    >
                                        Have Trouble Connecting?
                                    </div>
                                    <div
                                        style={{
                                            fontSize: 14,
                                            lineHeight: 1.5,
                                            width: 300,
                                            margin: "20px auto",
                                        }}
                                    >
                                        Boot your cloud PC first by selecting
                                        the "Launch My Cloud PC" button. You can
                                        also send a message to our support team{" "}
                                        <span
                                            className={styles.pointerOnHover}
                                            style={{ fontWeight: "bold" }}
                                            onClick={this.OpenSupport}
                                        >
                                            here
                                        </span>
                                        .
                                    </div>
                                </Popup>
                            )}
                        </div>
                        <div style={{ width: "50%", textAlign: "center" }}>
                            <Popup
                                trigger={
                                    <div
                                        className={styles.bigBox}
                                        style={{
                                            background:
                                                "linear-gradient(133.09deg, rgba(73, 238, 228, 0.8) 1.86%, rgba(109, 151, 234, 0.8) 100%)",
                                            borderRadius: 5,
                                            padding: 10,
                                            minHeight: 90,
                                            paddingTop: 20,
                                            paddingBottom: 0,
                                        }}
                                    >
                                        <FontAwesomeIcon
                                            icon={faUpload}
                                            style={{
                                                height: 40,
                                                color: "white",
                                            }}
                                        />
                                        <div
                                            style={{
                                                marginTop: 5,
                                                fontSize: 14,
                                                fontWeight: "bold",
                                                color: "white",
                                            }}
                                        >
                                            File Upload
                                        </div>
                                    </div>
                                }
                                modal
                                contentStyle={{
                                    width: 300,
                                    borderRadius: 5,
                                    backgroundColor: "white",
                                    border: "none",
                                    height: 100,
                                    padding: 20,
                                    textAlign: "center",
                                }}
                            >
                                <div
                                    style={{
                                        fontWeight: "bold",
                                        fontSize: 22,
                                        marginTop: 10,
                                    }}
                                    className={styles.blueGradient}
                                >
                                    <strong>Coming Soon</strong>
                                </div>
                                <div
                                    style={{
                                        fontSize: 14,
                                        lineHeight: 1.4,
                                        color: "#111111",
                                        marginTop: 20,
                                    }}
                                >
                                    Upload any folder to your cloud PC.
                                </div>
                            </Popup>
                        </div>
                    </div>
                </div>
            );
        } else if (this.props.currentWindow === "studios") {
            return (
                <div>
                    <div
                        style={{
                            position: "relative",
                            width: "100%",
                            height: 410,
                            borderRadius: 5,
                            boxShadow: "0px 4px 20px rgba(0, 0, 0, 0.2)",
                            background:
                                "linear-gradient(205.96deg, #363868 0%, #1E1F42 101.4%)",
                            overflowY: "scroll",
                        }}
                    >
                        <div
                            style={{
                                backgroundColor: "#161936",
                                padding: "20px 30px",
                                color: "white",
                                fontSize: 18,
                                fontWeight: "bold",
                                borderRadius: "5px 0px 0px 0px",
                                fontFamily: "Maven Pro",
                            }}
                        >
                            <div style={{ float: "left", display: "inline" }}>
                                <FontAwesomeIcon
                                    icon={faKeyboard}
                                    style={{
                                        height: 15,
                                        paddingRight: 4,
                                        position: "relative",
                                        bottom: 1,
                                    }}
                                />{" "}
                                My Computers
                            </div>
                            <div style={{ float: "right", display: "inline" }}>
                                <Popup
                                    trigger={
                                        <span>
                                            <FontAwesomeIcon
                                                className={
                                                    styles.pointerOnHover
                                                }
                                                icon={faInfoCircle}
                                                style={{
                                                    height: 20,
                                                    color: "#AAAAAA",
                                                }}
                                            />
                                        </span>
                                    }
                                    modal
                                    contentStyle={{
                                        width: 400,
                                        borderRadius: 5,
                                        backgroundColor: "#111111",
                                        border: "none",
                                        height: 100,
                                        padding: 30,
                                        textAlign: "center",
                                    }}
                                >
                                    <div
                                        style={{
                                            fontWeight: "bold",
                                            fontSize: 20,
                                        }}
                                        className={styles.blueGradient}
                                    >
                                        <strong>How It Works</strong>
                                    </div>
                                    <div
                                        style={{
                                            fontSize: 12,
                                            color: "#CCCCCC",
                                            marginTop: 20,
                                            lineHeight: 1.5,
                                        }}
                                    >
                                        This dashboard contains a list of all
                                        computers that you've installed Fractal
                                        onto. You can start a remote connection
                                        to any of these computers, as long `as
                                        they are turned on.
                                    </div>
                                </Popup>
                            </div>
                            <div style={{ display: "block", height: 20 }}></div>
                        </div>
                        {this.props.computers.map((value, index) => {
                            var defaultNickname = value.nickname;
                            return (
                                <div
                                    style={{
                                        padding: "30px 30px",
                                        borderBottom: "solid 0.5px #161936",
                                    }}
                                >
                                    <div style={{ display: "flex" }}>
                                        <div style={{ width: "69%" }}>
                                            {this.state.editNickname ===
                                            index ? (
                                                <div
                                                    className={
                                                        styles.nicknameContainer
                                                    }
                                                    style={{
                                                        color: "white",
                                                        fontSize: 14,
                                                        fontWeight: "bold",
                                                    }}
                                                >
                                                    <FontAwesomeIcon
                                                        className={
                                                            styles.pointerOnHover
                                                        }
                                                        onClick={() =>
                                                            this.NicknameEdit(
                                                                -1
                                                            )
                                                        }
                                                        icon={faCheck}
                                                        style={{
                                                            height: 14,
                                                            marginRight: 12,
                                                            position:
                                                                "relative",
                                                            width: 16,
                                                        }}
                                                    />
                                                    <input
                                                        defaultValue={
                                                            defaultNickname
                                                        }
                                                        type="text"
                                                        placeholder="Rename Computer"
                                                        onChange={
                                                            this.ChangeNickname
                                                        }
                                                        style={{
                                                            background: "none",
                                                            border:
                                                                "solid 0.5px #161936",
                                                            padding: "6px 10px",
                                                            borderRadius: 5,
                                                            outline: "none",
                                                            color: "white",
                                                        }}
                                                    />
                                                    <div
                                                        style={{ height: 12 }}
                                                    ></div>
                                                </div>
                                            ) : (
                                                <div>
                                                    <div
                                                        style={{
                                                            color: "white",
                                                            fontSize: 14,
                                                            fontWeight: "bold",
                                                        }}
                                                    >
                                                        <FontAwesomeIcon
                                                            className={
                                                                styles.pointerOnHover
                                                            }
                                                            onClick={() =>
                                                                this.NicknameEdit(
                                                                    index
                                                                )
                                                            }
                                                            icon={faPencilAlt}
                                                            style={{
                                                                height: 14,
                                                                marginRight: 12,
                                                                position:
                                                                    "relative",
                                                                width: 16,
                                                            }}
                                                        />
                                                        {value.nickname}
                                                    </div>
                                                    <div
                                                        style={{
                                                            fontSize: 12,
                                                            color: "#D6D6D6",
                                                            marginTop: 10,
                                                            marginLeft: 28,
                                                        }}
                                                    >
                                                        {value.location}
                                                    </div>
                                                </div>
                                            )}
                                        </div>
                                        <div style={{ width: "31%" }}>
                                            <div style={{ float: "right" }}>
                                                {value.id === this.props.id ? (
                                                    <div
                                                        style={{
                                                            width: 110,
                                                            textAlign: "center",
                                                            fontSize: 12,
                                                            fontWeight: "bold",
                                                            border: "none",
                                                            borderRadius: 5,
                                                            paddingTop: 7,
                                                            paddingBottom: 7,
                                                            background:
                                                                "#161936",
                                                            color: "#D6D6D6",
                                                        }}
                                                    >
                                                        This Computer
                                                    </div>
                                                ) : (
                                                    <div
                                                        className={
                                                            styles.pointerOnHover
                                                        }
                                                        style={{
                                                            width: 110,
                                                            textAlign: "center",
                                                            fontSize: 12,
                                                            fontWeight: "bold",
                                                            border: "none",
                                                            borderRadius: 5,
                                                            paddingTop: 7,
                                                            paddingBottom: 7,
                                                            background:
                                                                "#5EC4EB",
                                                            color: "white",
                                                        }}
                                                    >
                                                        Connect
                                                    </div>
                                                )}
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            );
                        })}
                    </div>
                </div>
            );
        } else if (this.props.currentWindow === "settings") {
            return (
                <div className={styles.settingsContainer}>
                    <div
                        style={{
                            position: "relative",
                            width: 800,
                            height: 410,
                            borderRadius: 5,
                            boxShadow: "0px 4px 20px rgba(0, 0, 0, 0.2)",
                            background: "white",
                            overflowY: "scroll",
                        }}
                    >
                        <div
                            style={{
                                backgroundColor: "#EFEFEF",
                                padding: "20px 30px",
                                color: "#111111",
                                fontSize: 18,
                                fontWeight: "bold",
                                borderRadius: "5px 0px 0px 0px",
                                fontFamily: "Maven Pro",
                            }}
                        >
                            <FontAwesomeIcon
                                icon={faCogs}
                                style={{
                                    height: 15,
                                    paddingRight: 4,
                                    position: "relative",
                                    bottom: 1,
                                }}
                            />{" "}
                            Settings
                        </div>
                        <div
                            style={{
                                padding: "30px 30px",
                                borderBottom: "solid 0.5px #EFEFEF",
                            }}
                        >
                            <div style={{ display: "flex" }}>
                                <div style={{ width: "75%" }}>
                                    <div
                                        style={{
                                            color: "#111111",
                                            fontSize: 16,
                                            fontWeight: "bold",
                                        }}
                                    >
                                        <img
                                            src={Window}
                                            style={{
                                                color: "#111111",
                                                height: 14,
                                                marginRight: 12,
                                                position: "relative",
                                                top: 2,
                                                width: 16,
                                            }}
                                        />
                                        Windowed Mode
                                    </div>
                                    <div
                                        style={{
                                            fontSize: 13,
                                            color: "#333333",
                                            marginTop: 10,
                                            marginLeft: 28,
                                            lineHeight: 1.4,
                                        }}
                                    >
                                        When activated, a titlebar will appear
                                        at the top of your cloud PC, so you can
                                        adjust your cloud PC's position on your
                                        screen.
                                    </div>
                                </div>
                                <div style={{ width: "25%" }}>
                                    <div style={{ float: "right" }}>
                                        <ToggleButton
                                            value={this.state.windowMode}
                                            onToggle={this.toggleWindow}
                                            colors={{
                                                active: {
                                                    base: "#5EC4EB",
                                                },
                                                inactive: {
                                                    base: "#161936",
                                                },
                                            }}
                                        />
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div
                            style={{
                                padding: "30px 30px",
                                borderBottom: "solid 0.5px #EFEFEF",
                            }}
                        >
                            <div style={{ display: "flex" }}>
                                <div style={{ width: "75%" }}>
                                    <div
                                        style={{
                                            color: "#111111",
                                            fontSize: 16,
                                            fontWeight: "bold",
                                        }}
                                    >
                                        <img
                                            src={Speedometer}
                                            style={{
                                                color: "#111111",
                                                height: 14,
                                                marginRight: 12,
                                                position: "relative",
                                                top: 2,
                                                width: 16,
                                            }}
                                        />
                                        Maximum Bandwidth
                                    </div>
                                    <div
                                        style={{
                                            fontSize: 13,
                                            color: "#333333",
                                            marginTop: 10,
                                            marginLeft: 28,
                                            lineHeight: 1.4,
                                        }}
                                    >
                                        Toggle the maximum bandwidth (Mbps) that
                                        Fractal consumes. We recommend adjusting
                                        this setting only if you are also
                                        running other, bandwidth-consuming apps.
                                    </div>
                                </div>
                                <div style={{ width: "25%" }}>
                                    <div style={{ float: "right" }}>
                                        <Slider
                                            axis="x"
                                            xstep={5}
                                            xmin={10}
                                            xmax={50}
                                            x={this.state.mbps}
                                            onChange={({ x }) =>
                                                this.changeMbps(x)
                                            }
                                            styles={{
                                                track: {
                                                    backgroundColor: "#161936",
                                                    height: 5,
                                                    width: 100,
                                                },
                                                active: {
                                                    backgroundColor: "#5EC4EB",
                                                    height: 5,
                                                },
                                                thumb: {
                                                    height: 10,
                                                    width: 10,
                                                },
                                            }}
                                        />
                                    </div>
                                    <br />
                                    <div
                                        style={{
                                            fontSize: 11,
                                            color: "#333333",
                                            float: "right",
                                            marginTop: 5,
                                        }}
                                    >
                                        {this.state.mbps < 50 ? (
                                            <div>{this.state.mbps} mbps</div>
                                        ) : (
                                            <div>Unlimited</div>
                                        )}
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div
                            style={{
                                padding: "30px 30px",
                                borderBottom: "solid 0.5px #EFEFEF",
                            }}
                        >
                            <div style={{ display: "flex" }}>
                                <div style={{ width: "75%" }}>
                                    <div
                                        style={{
                                            color: "#111111",
                                            fontSize: 16,
                                            fontWeight: "bold",
                                        }}
                                    >
                                        <img
                                            src={Scale}
                                            style={{
                                                color: "#111111",
                                                height: 14,
                                                marginRight: 12,
                                                position: "relative",
                                                top: 2,
                                                width: 16,
                                            }}
                                        />
                                        Scaling Factor
                                    </div>
                                    <div
                                        style={{
                                            fontSize: 13,
                                            color: "#333333",
                                            marginTop: 10,
                                            marginLeft: 28,
                                            lineHeight: 1.4,
                                        }}
                                    >
                                        To change the size of text and icons in
                                        your cloud PC, connect to your cloud PC,
                                        right click on the desktop, and select
                                        "Display Settings". Next, click on
                                        Screen 2. Finally, select the
                                        appropriate scaling factor (100%, 150%,
                                        etc.).
                                    </div>
                                </div>
                            </div>
                        </div>
                        <div style={{ padding: "30px 30px", marginBottom: 30 }}>
                            <div style={{ float: "right" }}>
                                <button
                                    onClick={this.ExitSettings}
                                    className={styles.signupButton}
                                    style={{ borderRadius: 5, width: 140 }}
                                >
                                    Save &amp; Exit
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            );
        }
    }
}

function mapStateToProps(state) {
    return {
        username: state.counter.username,
        isUser: state.counter.isUser,
        public_ip: state.counter.public_ip,
        ipInfo: state.counter.ipInfo,
        computers: state.counter.computers,
        fetchStatus: state.counter.fetchStatus,
        disk: state.counter.disk,
        attach_attempts: state.counter.attach_attempts,
        account_locked: state.counter.account_locked,
        promo_code: state.counter.promo_code,
        restart_status: state.counter.restart_status,
        restart_attempts: state.counter.restart_attempts,
        status_message: state.counter.status_message,
        ready_to_connect: state.counter.ready_to_connect,
    };
}

export default connect(mapStateToProps)(MainBox);
