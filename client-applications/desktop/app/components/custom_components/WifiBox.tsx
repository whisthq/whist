import React, { Component } from "react";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons";
import Style from "../../style/components/general.module.css";
const FastSpeedtest = require("fast-speedtest-api");

class WifiBox extends Component {
    constructor(props: any) {
        super(props);
        this.state = { downloadSpeed: 0, internetbar: 50 };
    }

    async MeasureConnectionSpeed() {
        let speedtest = new FastSpeedtest({
            token: "YXNkZmFzZGxmbnNkYWZoYXNkZmhrYWxm", // required
            verbose: false, // default: false
            timeout: 6000, // default: 5000
            https: true, // default: true
            urlCount: 5, // default: 5
            bufferSize: 8, // default: 8
            unit: FastSpeedtest.UNITS.Mbps, // default: Bps
        });
        speedtest
            .getSpeed()
            .then((s: any) => {
                this.setState({ downloadSpeed: s.toFixed(1) });
            })
            .catch((e: any) => {
                console.error(e.message);
            });
    }

    componentDidMount() {
        this.MeasureConnectionSpeed();
    }

    render() {
        let internetBox;

        if (this.state.downloadSpeed < 10) {
            internetBox = (
                <div>
                    <div
                        style={{
                            background: "none",
                            border: "solid 1px #d13628",
                            height: 6,
                            width: 6,
                            borderRadius: 4,
                            display: "inline",
                            float: "left",
                            position: "relative",
                            top: 5,
                            marginRight: 13,
                        }}
                    />
                    <h1 className={Style.header} style={{ marginTop: 5 }}>
                        {this.state.downloadSpeed} Mbps Download Speed
                    </h1>
                    {/* <h1 className={Style.header + " pl-3 ml-1"}>{this.state.uploadSpeed} Mbps Internet Up</h1>
          <h1 className={Style.header + " pl-3 ml-1"}>{this.state.ping} ms Ping</h1> */}
                    <p
                        className={Style.text}
                        style={{ marginTop: 8, paddingLeft: 20 }}
                    >
                        Your Internet bandwidth is slow. Try closing streaming
                        apps like Youtube, Netflix, or Spotify.
                    </p>
                </div>
            );
        } else if (this.state.downloadSpeed < 20) {
            internetBox = (
                <div>
                    <div
                        style={{
                            background: "none",
                            border: "solid 1px #f2a20c",
                            height: 6,
                            width: 6,
                            borderRadius: 4,
                            display: "inline",
                            float: "left",
                            position: "relative",
                            top: 5,
                            marginRight: 13,
                        }}
                    />
                    <h1 className={Style.header} style={{ marginTop: 5 }}>
                        {this.state.downloadSpeed} Mbps Download Speed
                    </h1>
                    {/* <h1 className={Style.header + " pl-3 ml-1"}>{this.state.uploadSpeed} Mbps Internet Up</h1>
          <h1 className={Style.header + " pl-3 ml-1"}>{this.state.ping} ms Ping</h1> */}
                    <p
                        className={Style.text}
                        style={{ marginTop: 8, paddingLeft: 20 }}
                    >
                        Expect a smooth streaming experience with occassional
                        image quality drops.
                    </p>
                </div>
            );
        } else {
            internetBox = (
                <div>
                    <div
                        style={{
                            background: "none",
                            border: "solid 1px #14a329",
                            height: 6,
                            width: 6,
                            borderRadius: 4,
                            display: "inline",
                            float: "left",
                            position: "relative",
                            top: 5,
                            marginRight: 13,
                        }}
                    />
                    <h1 className={Style.header} style={{ marginTop: 5 }}>
                        {this.state.downloadSpeed} Mbps Download Speed
                    </h1>
                    {/* <h1 className={Style.header + " pl-3 ml-1"}>{this.state.uploadSpeed} Mbps Internet Up</h1>
          <h1 className={Style.header + " pl-3 ml-1"}>{this.state.ping} ms Ping</h1> */}
                    <p
                        className={Style.text}
                        style={{ marginTop: 8, paddingLeft: 20 }}
                    >
                        Your Internet is fast enough to support high-quality
                        streaming.
                    </p>
                </div>
            );
        }

        return (
            <div style={{ marginTop: 15 }}>
                {this.state.downloadSpeed === 0 ? (
                    <div
                        style={{
                            marginTop: 25,
                            position: "relative",
                            right: 5,
                        }}
                    >
                        <FontAwesomeIcon
                            icon={faCircleNotch}
                            spin
                            style={{
                                color: "#5EC4EB",
                                height: 10,
                                display: "inline",
                                marginRight: 9,
                            }}
                        />
                        <p
                            className={Style.text}
                            style={{ marginTop: 8, display: "inline" }}
                        >
                            Checking Internet speed
                        </p>
                    </div>
                ) : (
                    <div>
                        <div style={{ height: 10 }}></div>
                        {internetBox}
                    </div>
                )}
            </div>
        );
    }
}

export default WifiBox;
