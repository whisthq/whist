import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { history } from "store/configureStore";

import Titlebar from "react-electron-titlebar";
import Background from "assets/images/background.jpg";
import Logo from "assets/images/logo.svg";
import UpdateScreen from "pages/dashboard/components/update";

import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import {
    faCircleNotch,
    faUser,
    faLock,
} from "@fortawesome/free-solid-svg-icons";

import { FaGoogle } from "react-icons/fa";

import {
    loginUser,
    setOS,
    loginFailed,
    googleLogin,
} from "store/actions/counter_actions";

import { GOOGLE_CLIENT_ID } from "constants/config";

import "styles/login.css";

const Login = (props: any) => {
    const { dispatch, public_ip, os, warning } = props;

    const [username, setUsername] = useState("");
    const [password, setPassword] = useState("");
    const [loggingIn, setLoggingIn] = useState(false);
    const [version, setVersion] = useState("1.0.0");
    const [rememberMe, setRememberMe] = useState(false);
    const live = useState(true);
    const [updatePingReceived, setUpdatePingReceived] = useState(false);
    const [needsAutoupdate, setNeedsAutoupdate] = useState(false);

    const updateUsername = (evt: any) => {
        setUsername(evt.target.value);
    };

    const updatePassword = (evt: any) => {
        setPassword(evt.target.value);
    };

    const handleLoginUser = () => {
        const storage = require("electron-json-storage");
        dispatch(loginFailed(false));
        setLoggingIn(true);
        if (rememberMe) {
            storage.set("credentials", {
                username: username,
                password: password,
            });
        } else {
            storage.set("credentials", { username: "", password: "" });
        }
        dispatch(loginUser(username.trim(), password));
    };

    const loginKeyPress = (event: any) => {
        if (event.key === "Enter") {
            handleLoginUser();
        }
    };

    const handleGoogleLogin = () => {
        const { BrowserWindow } = require("electron").remote;

        const authWindow = new BrowserWindow({
            width: 800,
            height: 600,
            show: false,
            "node-integration": false,
            "web-security": false,
        });
        const authUrl = `https://accounts.google.com/o/oauth2/v2/auth?scope=openid%20profile%20email&openid.realm&include_granted_scopes=true&response_type=code&redirect_uri=urn:ietf:wg:oauth:2.0:oob:auto&client_id=${GOOGLE_CLIENT_ID}&origin=https%3A//fractalcomputers.com`;
        authWindow.loadURL(authUrl, { userAgent: "Chrome" });
        authWindow.show();

        authWindow.webContents.on("page-title-updated", () => {
            const pageTitle = authWindow.getTitle();
            if (pageTitle.includes("Success")) {
                const codeRegexp = new RegExp(
                    "^(?:Success code=)(.+?)(?:&.+)$"
                );
                const code = pageTitle.match(codeRegexp)[1];
                setLoggingIn(true);
                dispatch(googleLogin(code));
            }
        });
    };

    const forgotPassword = () => {
        const { shell } = require("electron");
        shell.openExternal("https://www.fractalcomputers.com/reset");
    };

    const signUp = () => {
        const { shell } = require("electron");
        shell.openExternal("https://www.fractalcomputers.com/auth");
    };

    const changeRememberMe = (event: any) => {
        setRememberMe(event.target.checked);
    };

    useEffect(() => {
        const ipc = require("electron").ipcRenderer;
        const storage = require("electron-json-storage");

        ipc.on("update", (_: any, update: any) => {
            setUpdatePingReceived(true);
            setNeedsAutoupdate(update);
        });

        const appVersion = require("../../package.json").version;
        const os = require("os");
        dispatch(setOS(os.platform()));
        setVersion(appVersion);

        storage.get("credentials", function (error: any, data: any) {
            if (error) throw error;

            if (data && Object.keys(data).length > 0) {
                if (data.username != "" && data.password != "" && live) {
                    setUsername(data.username);
                    setPassword(data.password);
                    setLoggingIn(true);
                }
            }
        });

        if (username && public_ip && live) {
            history.push("/dashboard");
        }
    }, []);

    useEffect(() => {
        const sleep = (milliseconds: number) => {
            return new Promise((resolve) => setTimeout(resolve, milliseconds));
        };

        const wait_for_autoupdate = async () => {
            await sleep(2000);
        };

        while (!updatePingReceived) {
            wait_for_autoupdate();
        }

        if (updatePingReceived && !needsAutoupdate) {
            wait_for_autoupdate();
            dispatch(loginUser(username, password));
        }
    }, [username, password, loggingIn]);

    return (
        <div
            className="container"
            data-tid="container"
            style={{ backgroundImage: `url(${Background})` }}
        >
            <UpdateScreen />
            <div
                style={{
                    position: "absolute",
                    bottom: 15,
                    right: 15,
                    fontSize: 11,
                    color: "#D1D1D1",
                }}
            >
                Version: {version}
            </div>
            {os === "win32" ? (
                <div>
                    <Titlebar backgroundColor="#000000" />
                </div>
            ) : (
                <div className="macTitleBar" />
            )}
            {live ? (
                <div className="removeDrag">
                    <div className="landingHeader">
                        <div className="landingHeaderLeft">
                            <img src={Logo} width="18" height="18" />
                            <span className="logoTitle">Fractal</span>
                        </div>
                        <div className="landingHeaderRight">
                            <span id="forgotButton" onClick={forgotPassword}>
                                Forgot Password?
                            </span>
                            <button
                                type="button"
                                className="signupButton"
                                style={{ borderRadius: 5, marginLeft: 15 }}
                                id="signup-button"
                                onClick={signUp}
                            >
                                Sign Up
                            </button>
                        </div>
                    </div>
                    <div style={{ marginTop: warning ? 10 : 60 }}>
                        <div className="loginContainer">
                            <div>
                                <FontAwesomeIcon
                                    icon={faUser}
                                    style={{
                                        color: "white",
                                        fontSize: 12,
                                    }}
                                    className="inputIcon"
                                />
                                <input
                                    onKeyPress={loginKeyPress}
                                    onChange={updateUsername}
                                    type="text"
                                    className="inputBox"
                                    style={{ borderRadius: 5 }}
                                    placeholder="Username"
                                    id="username"
                                />
                            </div>
                            <div>
                                <FontAwesomeIcon
                                    icon={faLock}
                                    style={{
                                        color: "white",
                                        fontSize: 12,
                                    }}
                                    className="inputIcon"
                                />
                                <input
                                    onKeyPress={loginKeyPress}
                                    onChange={updatePassword}
                                    type="password"
                                    className="inputBox"
                                    style={{ borderRadius: 5 }}
                                    placeholder="Password"
                                    id="password"
                                />
                            </div>
                            <div style={{ marginBottom: 20 }}>
                                {loggingIn && !warning ? (
                                    <button
                                        type="button"
                                        className="loginButton"
                                        id="login-button"
                                        style={{
                                            opacity: 0.6,
                                            textAlign: "center",
                                        }}
                                    >
                                        <FontAwesomeIcon
                                            icon={faCircleNotch}
                                            spin
                                            style={{
                                                color: "white",
                                                width: 12,
                                                marginRight: 5,
                                                position: "relative",
                                                top: 0.5,
                                            }}
                                        />{" "}
                                        Processing
                                    </button>
                                ) : (
                                    <button
                                        onClick={handleLoginUser}
                                        type="button"
                                        className="loginButton"
                                        id="login-button"
                                    >
                                        START
                                    </button>
                                )}
                                <div style={{ marginBottom: 20 }}>
                                    {loggingIn && !warning ? (
                                        <button
                                            type="button"
                                            className="googleButton"
                                            id="google-button"
                                            style={{
                                                opacity: 0.6,
                                                textAlign: "center",
                                            }}
                                        >
                                            <FontAwesomeIcon
                                                icon={faCircleNotch}
                                                spin
                                                style={{
                                                    color: "white",
                                                    width: 12,
                                                    marginRight: 5,
                                                    position: "relative",
                                                    top: 0.5,
                                                }}
                                            />{" "}
                                            Processing
                                        </button>
                                    ) : (
                                        <button
                                            onClick={handleGoogleLogin}
                                            type="button"
                                            className="googleButton"
                                            id="google-button"
                                        >
                                            <FaGoogle
                                                style={{
                                                    fontSize: 16,
                                                    marginRight: 10,
                                                    position: "relative",
                                                    top: 3,
                                                }}
                                            />
                                            Login with Google
                                        </button>
                                    )}
                                </div>
                            </div>
                            {warning && (
                                <div
                                    style={{
                                        textAlign: "center",
                                        fontSize: 12,
                                        color: "#f9000b",
                                        background: "rgba(253, 240, 241, 0.9)",
                                        width: "100%",
                                        padding: 15,
                                        borderRadius: 2,
                                        margin: "auto",
                                        marginBottom: 30,
                                        // width: 265,
                                    }}
                                >
                                    <div>
                                        Invalid credentials. If you lost your
                                        password, you can reset it on the&nbsp;
                                        <div
                                            onClick={forgotPassword}
                                            className="pointerOnHover"
                                            style={{
                                                display: "inline",
                                                fontWeight: "bold",
                                                textDecoration: "underline",
                                            }}
                                        >
                                            website
                                        </div>
                                        .
                                    </div>
                                </div>
                            )}
                            <div
                                style={{
                                    marginTop: 25,
                                    display: "flex",
                                    justifyContent: "center",
                                    alignItems: "center",
                                }}
                            >
                                <label className="termsContainer">
                                    <input
                                        type="checkbox"
                                        onChange={changeRememberMe}
                                        onKeyPress={loginKeyPress}
                                    />
                                    <span className="checkmark" />
                                </label>

                                <div style={{ fontSize: 12 }}>Remember Me</div>
                            </div>
                        </div>
                    </div>
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
                    We are currently pushing out a critical Linux update. Your
                    app will be back online very soon. We apologize for the
                    inconvenience!
                </div>
            )}
        </div>
    );
};

function mapStateToProps(state: any) {
    return {
        username: state.counter.username,
        public_ip: state.counter.public_ip,
        warning: state.counter.warning,
        os: state.counter.os,
    };
}

export default connect(mapStateToProps)(Login);
