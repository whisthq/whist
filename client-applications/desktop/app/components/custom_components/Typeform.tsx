import React, { Component } from "react";
import * as geolib from "geolib";
import { connect } from "react-redux";
import styles from "../Counter.css";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCheck, faArrowRight } from "@fortawesome/free-solid-svg-icons";

import {
    resetFeedback,
    sendFeedback,
    askFeedback,
} from "../../actions/counter";

class Typeform extends Component {
    constructor(props) {
        super(props);
        this.state = { feedback: "", feedbackThankYou: false };
    }

    UpdateFeedback = (evt) => {
        this.setState(
            {
                feedback: evt.target.value,
            },
            function () {
                console.log(this.state.feedback);
            }
        );
    };

    _HandleKeyDown = (evt) => {
        if (this.props.askFeedback) {
            if (evt.key === "Enter") {
                if (this.state.feedback) {
                    this.setState({ feedbackThankYou: true });
                } else {
                    this.setState({ feedbackThankYou: false });
                }
                this.props.dispatch(askFeedback(false));
            }
        } else if (this.state.feedbackThankYou) {
            if (evt.key === "Enter") {
                this.setState({ feedbackThankYou: false });
                this.props.dispatch(askFeedback(false));
                if (this.state.feedback) {
                    this.props.dispatch(sendFeedback(this.state.feedback));
                }
            } else {
                this.setState({ feedbackThankYou: false });
                this.props.dispatch(askFeedback(true));
            }
        }
    };

    ForwardFeedbackButton = () => {
        if (this.state.feedback) {
            this.setState({ feedbackThankYou: true });
        } else {
            this.setState({ feedbackThankYou: false });
        }
        this.props.dispatch(askFeedback(false));
    };

    SubmitFeedbackButton = () => {
        this.setState({ feedbackThankYou: false });
        this.props.dispatch(askFeedback(false));
        if (this.state.feedback) {
            this.props.dispatch(sendFeedback(this.state.feedback));
        }
    };

    componentDidMount() {
        document.addEventListener("keydown", this._HandleKeyDown);
    }

    componentDidUpdate(prevProps) {
        if (this.props.resetFeedback) {
            this.setState({ feedback: "" });
            this.props.dispatch(resetFeedback(false));
        }
    }

    render() {
        return (
            <div>
                {this.props.askFeedback ? (
                    <div
                        onKeyDown={this.ForwardFeedback}
                        style={{
                            position: "absolute",
                            top: 0,
                            left: 0,
                            width: 900,
                            height: 600,
                            background: "#0B172B",
                            zIndex: 2,
                            textAlign: "left",
                        }}
                    >
                        <div
                            style={{ padding: 150 }}
                            className={styles.feedbackBox}
                        >
                            <div
                                style={{
                                    fontSize: 24,
                                    color: "#5EC4EB",
                                    fontWeight: "bold",
                                }}
                            >
                                How Was Your Experience?
                            </div>
                            <div
                                style={{
                                    marginTop: 20,
                                    fontSize: 15,
                                    color: "#5EC4EB",
                                    opacity: 0.6,
                                }}
                            >
                                Our engineers rely on the bugs, feature
                                suggestions, and general feedback you provide to
                                build a better product!
                            </div>
                            <input
                                type="text"
                                value={this.state.feedback}
                                placeholder="Type your answer here"
                                onChange={this.UpdateFeedback}
                                style={{
                                    marginTop: 35,
                                    width: 600,
                                    background: "none",
                                    border: "none",
                                    borderBottom: "solid 0.5px #5EC4EB",
                                    outline: "none",
                                    padding: "10px 10px 10px 0px",
                                    fontSize: 24,
                                    color: "#5EC4EB",
                                }}
                            ></input>
                            {this.state.feedback === "" ? (
                                <div>
                                    <button
                                        onClick={this.ForwardFeedbackButton}
                                        className={styles.feedbackButton}
                                        style={{
                                            display: "inline",
                                            width: 100,
                                        }}
                                    >
                                        SKIP
                                        <FontAwesomeIcon
                                            icon={faCheck}
                                            style={{
                                                paddingLeft: 8,
                                                height: 14,
                                            }}
                                        />
                                    </button>
                                    <div
                                        style={{
                                            display: "inline",
                                            fontSize: 12,
                                            marginTop: 40,
                                            color: "#5EC4EB",
                                            marginLeft: 12,
                                        }}
                                    >
                                        press <strong>Enter</strong>
                                        <FontAwesomeIcon
                                            icon={faArrowRight}
                                            style={{
                                                paddingLeft: 6,
                                                height: 10,
                                                position: "relative",
                                            }}
                                        />
                                    </div>
                                </div>
                            ) : (
                                <div>
                                    <button
                                        onClick={this.ForwardFeedbackButton}
                                        className={styles.feedbackButton}
                                        style={{ display: "inline" }}
                                    >
                                        OK
                                        <FontAwesomeIcon
                                            icon={faCheck}
                                            style={{
                                                paddingLeft: 8,
                                                height: 14,
                                            }}
                                        />
                                    </button>
                                    <div
                                        style={{
                                            display: "inline",
                                            fontSize: 12,
                                            marginTop: 40,
                                            color: "#5EC4EB",
                                            marginLeft: 12,
                                        }}
                                    >
                                        press <strong>Enter</strong>
                                        <FontAwesomeIcon
                                            icon={faArrowRight}
                                            style={{
                                                paddingLeft: 6,
                                                height: 10,
                                                position: "relative",
                                            }}
                                        />
                                    </div>
                                </div>
                            )}
                        </div>
                    </div>
                ) : (
                    <div></div>
                )}
                {this.state.feedbackThankYou ? (
                    <div
                        onKeyDown={this.SubmitFeedback}
                        style={{
                            position: "absolute",
                            top: 0,
                            left: 0,
                            width: 900,
                            height: 600,
                            background: "#0B172B",
                            zIndex: 3,
                            textAlign: "left",
                        }}
                    >
                        <div style={{ padding: "300px 150px" }}>
                            <div
                                style={{
                                    width: 900,
                                    height: 1,
                                    position: "relative",
                                    right: 150,
                                    background: "#5EC4EB",
                                    opacity: 0.4,
                                    marginBottom: 30,
                                }}
                            ></div>
                            <button
                                onClick={this.SubmitFeedbackButton}
                                className={styles.feedbackButton}
                                style={{ display: "inline", width: 120 }}
                            >
                                SUBMIT
                                <FontAwesomeIcon
                                    icon={faCheck}
                                    style={{ paddingLeft: 8, height: 14 }}
                                />
                            </button>
                            <div
                                style={{
                                    display: "inline",
                                    fontSize: 12,
                                    marginTop: 40,
                                    color: "#5EC4EB",
                                    marginLeft: 12,
                                }}
                            >
                                press <strong>Enter</strong>
                                <FontAwesomeIcon
                                    icon={faArrowRight}
                                    style={{
                                        paddingLeft: 6,
                                        height: 10,
                                        position: "relative",
                                        bottom: 1,
                                    }}
                                />
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

function mapStateToProps(state) {
    return {
        askFeedback: state.counter.askFeedback,
        resetFeedback: state.counter.resetFeedback,
    };
}

export default connect(mapStateToProps)(Typeform);
