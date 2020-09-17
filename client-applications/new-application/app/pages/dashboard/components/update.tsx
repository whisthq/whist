import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import Titlebar from "react-electron-titlebar";
import Logo from "assets/images/logo.svg";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons";

import "styles/login.css";

const UpdateScreen = (props: any) => {
    const { os } = props;

    const [updateScreen, setUpdateScreen] = useState(false);
    const [percentageLeft, setPercentageLeft] = useState(300);
    const [percentageDownloaded, setPercentageDownloaded] = useState(0);
    const [downloadSpeed, setDownloadSpeed] = useState("0");
    const [transferred, setTransferred] = useState("0");
    const [total, setTotal] = useState("0");
    const [downloadError, setDownloadError] = useState("");

    const forgotPassword = () => {
        const { shell } = require("electron");
        shell.openExternal("https://www.fractalcomputers.com/reset");
    };

    const signUp = () => {
        const { shell } = require("electron");
        shell.openExternal("https://www.fractalcomputers.com/auth");
    };

    useEffect(() => {
        const ipc = require("electron").ipcRenderer;

        ipc.on("update", (_: any, update: any) => {
            if (update) {
                setUpdateScreen(true);
            }
        });

        ipc.on("percent", (_: any, percent: any) => {
            percent = percent * 3;
            setPercentageLeft(300 - percent);
            setPercentageDownloaded(percent);
        });

        ipc.on("download-speed", (_: any, speed: any) => {
            setDownloadSpeed((speed / 1000000).toFixed(2));
        });

        ipc.on("transferred", (_: any, transferred: any) => {
            setTransferred((transferred / 1000000).toFixed(2));
        });

        ipc.on("total", (_: any, total: any) => {
            setTotal((total / 1000000).toFixed(2));
        });

        ipc.on("error", (_: any, error: any) => {
            setDownloadError(error);
        });

        // ipc.on("downloaded", (event, downloaded) => {});
    }, []);

    return (
        <div>
            {updateScreen ? (
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
                    {os === "win32" ? (
                        <div>
                            <Titlebar backgroundColor="#000000" />
                        </div>
                    ) : (
                        <div style={{ marginTop: 10 }}></div>
                    )}
                    <div className="landingHeader">
                        <div className="landingHeaderLeft">
                            <img src={Logo} width="20" height="20" />
                            <span className="logoTitle">Fractal</span>
                        </div>
                        <div className="landingHeaderRight">
                            <span id="forgotButton" onClick={forgotPassword}>
                                Forgot Password?
                            </span>
                            <button
                                type="button"
                                className="signupButton"
                                id="signup-button"
                                onClick={signUp}
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
                                width: 1000,
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
                                        width: `${percentageDownloaded}px`,
                                        height: 6,
                                        background:
                                            "linear-gradient(258.54deg, #5ec3eb 0%, #d023eb 100%)",
                                    }}
                                ></div>
                                <div
                                    style={{
                                        width: `${percentageLeft}px`,
                                        height: 6,
                                        background: "#111111",
                                    }}
                                ></div>
                            </div>
                            {downloadError === "" ? (
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
                                        Downloading Update ({downloadSpeed}{" "}
                                        Mbps)
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
                                        {downloadError}
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
                                {transferred} / {total} MB Downloaded
                            </div>
                        </div>
                    </div>
                </div>
            ) : (
                <div></div>
            )}
        </div>
    );
};

function mapStateToProps(state: any) {
    return {
        os: state.counter.os,
    };
}

export default connect(mapStateToProps)(UpdateScreen);
