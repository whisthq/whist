import React, { Component } from "react";
import { connect } from "react-redux";
import styles from "pages/PageDashboard/Dashboard.css";
import { history } from "store/configureStore";

import Titlebar from "react-electron-titlebar";
import Logo from "resources/images/logo.svg";
import WifiBox from "pages/PageDashboard/components/WifiBox.tsx";
import DistanceBox from "pages/PageDashboard/components/DistanceBox.tsx";
import CPUBox from "pages/PageDashboard/components/CPUBox.tsx";
import Typeform from "pages/PageDashboard/components/Typeform.tsx";
import MainBox from "pages/PageDashboard/components/MainBox.tsx";
import UpdateScreen from "pages/PageDashboard/components/UpdateScreen.tsx";

import Popup from "reactjs-popup";

import { askFeedback, changeWindow, logout } from "actions/counter";

class Dashboard extends Component {
    constructor(props) {
        super(props);
        this.state = {
            isLoading: true,
            username: "",
            launches: 0,
            showTitlebar: true,
            location_ip: "",
        };
    }

    CloseWindow = () => {
        const remote = require("electron").remote;
        let win = remote.getCurrentWindow();

        win.close();
    };

    MinimizeWindow = () => {
        const remote = require("electron").remote;
        let win = remote.getCurrentWindow();

        win.minimize();
    };

    OpenFeedback = () => {
        this.setState({ showTitlebar: false });
        this.props.dispatch(askFeedback(true));
    };

    OpenSettings = () => {
        this.props.dispatch(changeWindow("settings"));
    };

    ReturnToDashboard = () => {
        this.props.dispatch(changeWindow("main"));
    };

    LogOut = () => {
        this.props.dispatch(logout());
        const storage = require("electron-json-storage");
        storage.set("credentials", { username: "", password: "" }, function (
            err
        ) {
            history.push("/");
        });
    };

    componentDidMount() {
        this.props.dispatch(changeWindow("main"));
        this.setState({ isLoading: false });
        if (this.props.username && this.props.username != "") {
            this.setState({ username: this.props.username.split("@")[0] });
        }
    }

    componentDidUpdate(prevProps) {
        if (
            this.props.username &&
            prevProps.username == "" &&
            this.props.username != "" &&
            this.state.username == ""
        ) {
            this.setState({ username: this.props.username.split("@")[0] });
        }
        if (
            prevProps.askFeedback &&
            !this.props.askFeedback &&
            !this.state.showTitlebar
        ) {
            this.setState({ showTitlebar: true });
        }
        if (this.props.askFeedback && this.state.showTitlebar) {
            this.setState({ showTitlebar: false });
        }
        if (this.props.location != "" && this.state.location_ip == "") {
            if (this.props.location == "eastus") {
                this.setState({ location_ip: "40.76.207.99" });
            } else if (this.props.location == "southcentralus") {
                this.setState({ location_ip: "104.215.96.85" });
            } else {
                this.setState({ location_ip: "65.52.6.169" });
            }
        }
    }

    render() {
        const barHeight = 5;

        return (
            <div
                className={styles.container}
                data-tid="container"
                style={{ fontFamily: "Maven Pro" }}
            >
                <UpdateScreen />
                {this.props.os === "win32" ? (
                    <div style={{ marginBottom: 20 }}>
                        {this.state.showTitlebar ? (
                            <div
                                style={{
                                    backgroundColor: "#EEEEEE",
                                    width: 30,
                                    height: 28,
                                    position: "absolute",
                                    top: 0,
                                    right: 125,
                                    zIndex: 100,
                                }}
                            ></div>
                        ) : (
                            <div></div>
                        )}
                        <div
                            style={{
                                backgroundColor: " rgb(94, 195, 235)",
                                width: 150,
                                position: "absolute",
                                top: 0,
                                right: 0,
                            }}
                        >
                            <Titlebar />
                        </div>
                    </div>
                ) : (
                    <div className={styles.macTitleBar} />
                )}
                {this.state.isLoading ? (
                    <div></div>
                ) : (
                    <div>
                        <Typeform />
                        <div className={styles.removeDrag}>
                            <div className={styles.landingHeader}>
                                <div
                                    className={styles.landingHeaderLeft}
                                    onClick={this.ReturnToDashboard}
                                >
                                    <div className={styles.pointerOnHover}>
                                        <img
                                            src={Logo}
                                            width="20"
                                            height="20"
                                        />
                                        <span
                                            className={styles.logoTitle}
                                            style={{
                                                color: "#111111",
                                                fontWeight: "bold",
                                            }}
                                        >
                                            Fractal
                                        </span>
                                    </div>
                                </div>
                                <div className={styles.landingHeaderRight}>
                                    <span
                                        className={styles.headerButton}
                                        onClick={this.OpenFeedback}
                                    >
                                        Support
                                    </span>
                                    <span
                                        className={styles.headerButton}
                                        onClick={this.OpenSettings}
                                    >
                                        Settings
                                    </span>
                                    <Popup
                                        trigger={
                                            <span
                                                className={styles.headerButton}
                                            >
                                                How to Use
                                            </span>
                                        }
                                        modal
                                        contentStyle={{
                                            width: 450,
                                            color: "#111111",
                                            borderRadius: 5,
                                            backgroundColor: "white",
                                            border: "none",
                                            height: 450,
                                            padding: 30,
                                            textAlign: "left",
                                            overflowY: "scroll",
                                            lineHeight: 1.6,
                                        }}
                                    >
                                        <div
                                            style={{
                                                fontSize: 28,
                                                marginTop: 20,
                                                marginBottom: 10,
                                                fontWeight: "bold",
                                                textAlign: "center",
                                            }}
                                            className={styles.blueGradient}
                                        >
                                            How to Use
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            Welcome to Fractal! To get started,
                                            simply click the{" "}
                                            <b>Launch My Cloud PC</b> button.
                                            Fractal is a computer in the cloud,
                                            and is used just like your regular
                                            computer. Here's a few useful things
                                            to help make your experience the
                                            best it can be:
                                        </div>
                                        <div className={styles.subTitle}>
                                            Exiting your Fractal PC
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            You can exit your Fractal PC by
                                            clicking on the Windows Start Menu
                                            and then clicking on the{" "}
                                            <b>Exit Fractal</b> shortcut under
                                            the <b>&</b> sign, atop the menu
                                            bar. There's also a similar exit
                                            button on the desktop.
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            You can also exit via the shortchut{" "}
                                            <b>Ctrl+Alt+F4</b>, on Windows
                                            devices, and <b>Cmd+Ctrl+F4</b>, on
                                            MacOS devices.
                                        </div>
                                        <div className={styles.subTitle}>
                                            Clipboard
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            Fractal contains a bidirectional
                                            clipboard between your local
                                            computer and your Fractal PC. You
                                            can copy-paste text, images and
                                            files seamlessly, and Fractal will
                                            transfer this data back-and-forth.
                                            This feature works best in windowed
                                            mode.
                                        </div>
                                        <div className={styles.subTitle}>
                                            Windowed Mode
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            You can launch Fractal in windowed
                                            mode and resize the window to the
                                            size you want by dragging on the
                                            corners of the window, just like any
                                            other program. The stream of your
                                            Fractal PC will freeze as you drag,
                                            and pick up seamlessly when you
                                            release.
                                        </div>
                                        <div className={styles.subTitle}>
                                            Scaling Factor
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            You can modify the scaling factor of
                                            your Fractal PC to support all the
                                            way up to 4K. Beware that this will
                                            increase bandwidth consumption.
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            To change the size of text and icons
                                            in your cloud PC, connect to your
                                            cloud PC, right click on the desktop
                                            and select "Display Settings". Next,
                                            click on Screen 2. Finally, select
                                            the appropriate scaling factor
                                            (100%, 150%, etc.).
                                        </div>
                                        <div className={styles.subTitle}>
                                            Current Limitations
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 450,
                                                margin: "auto",
                                                marginTop: 15,
                                                fontSize: 14,
                                            }}
                                        >
                                            We are working diligently to improve
                                            Fractal. However, there are a few
                                            limitations outside of our control.
                                        </div>
                                        <div>
                                            <div
                                                className={styles.limitationBox}
                                            >
                                                The graphics cards powering
                                                Fractal PCs do not currently
                                                support resolutions compatible
                                                with MacOS resolutions, and
                                                might appear slightly stretched
                                                on those devices.
                                            </div>
                                            <div
                                                className={styles.limitationBox}
                                            >
                                                Some video games and softwares
                                                have trouble communicating with
                                                a cloud PC. If you experience
                                                this issue, please report the
                                                specific software to{" "}
                                                <b>
                                                    support@fractalcomputers.com
                                                </b>{" "}
                                                so we can communicate with the
                                                publisher.
                                            </div>
                                            <div
                                                className={styles.limitationBox}
                                            >
                                                VPNs should not be used on
                                                Fractal PCs as they will prevent
                                                the Fractal PC from
                                                communicating with your local
                                                computer.
                                            </div>
                                            <div
                                                className={styles.limitationBox}
                                            >
                                                Docker does not run on Fractal
                                                PCs, since nested virtualization
                                                is not currently supported on
                                                the hardware we use.
                                            </div>
                                        </div>
                                    </Popup>
                                    <Popup
                                        trigger={
                                            <span
                                                className={styles.headerButton}
                                            >
                                                Refer a Friend
                                            </span>
                                        }
                                        modal
                                        contentStyle={{
                                            width: 350,
                                            color: "#111111",
                                            borderRadius: 5,
                                            backgroundColor: "white",
                                            border: "none",
                                            height: 150,
                                            padding: 30,
                                            textAlign: "center",
                                        }}
                                    >
                                        <div
                                            style={{
                                                fontWeight: "bold",
                                                fontSize: 16,
                                            }}
                                        >
                                            Your Referral Code
                                        </div>
                                        <div
                                            style={{
                                                fontSize: 36,
                                                marginTop: 20,
                                            }}
                                            className={styles.blueGradient}
                                        >
                                            {this.props.promo_code}
                                        </div>
                                        <div
                                            style={{
                                                maxWidth: 275,
                                                margin: "auto",
                                                marginTop: 25,
                                                fontSize: 14,
                                                lineHeight: 1.4,
                                            }}
                                        >
                                            Share it with a friend and get a
                                            free month when they create a cloud
                                            PC.
                                        </div>
                                    </Popup>
                                    <button
                                        onClick={this.LogOut}
                                        type="button"
                                        className={styles.signupButton}
                                        id="signup-button"
                                        style={{
                                            marginLeft: 25,
                                            fontFamily: "Maven Pro",
                                            fontWeight: "bold",
                                        }}
                                    >
                                        Sign Out
                                    </button>
                                </div>
                            </div>
                            <div
                                style={{
                                    display: "flex",
                                    padding: "20px 50px",
                                }}
                            >
                                <div
                                    style={{
                                        width: "70%",
                                        textAlign: "left",
                                        paddingRight: 20,
                                    }}
                                >
                                    <MainBox
                                        currentWindow={this.props.currentWindow}
                                        default="main"
                                    />
                                </div>
                                {this.props.currentWindow === "main" ? (
                                    <div
                                        className={styles.statBox}
                                        style={{
                                            width: "40%",
                                            textAlign: "left",
                                            background: "white",
                                            borderRadius: 5,
                                            minHeight: 350,
                                            color: "#111111",
                                        }}
                                    >
                                        <div
                                            style={{
                                                fontWeight: "bold",
                                                fontSize: 21,
                                                lineHeight: 1.3,
                                                paddingLeft: 30,
                                                paddingRight: 30,
                                                paddingTop: 30,
                                                borderRadius: "5px 5px 0px 0px",
                                                paddingBottom: 10,
                                            }}
                                        >
                                            <span
                                                className={styles.blueGradient}
                                            >
                                                Welcome, {this.state.username}
                                            </span>
                                        </div>
                                        <div
                                            style={{
                                                paddingLeft: 30,
                                                paddingRight: 30,
                                                paddingBottom: 25,
                                            }}
                                        >
                                            <WifiBox barHeight={barHeight} />
                                            <CPUBox barHeight={barHeight} />
                                            <DistanceBox
                                                barHeight={barHeight}
                                                public_ip={
                                                    this.state.location_ip
                                                }
                                            />
                                        </div>
                                    </div>
                                ) : (
                                    <div></div>
                                )}
                            </div>
                        </div>
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
        os: state.counter.os,
        askFeedback: state.counter.askFeedback,
        currentWindow: state.counter.window,
        disk: state.counter.disk,
        promo_code: state.counter.promo_code,
        location: state.counter.location,
    };
}

export default connect(mapStateToProps)(Dashboard);
