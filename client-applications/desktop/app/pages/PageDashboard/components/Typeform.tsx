import React, { Component } from "react";
import { connect } from "react-redux";
import styles from "pages/PageDashboard/Dashboard.css";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faCheck, faArrowRight } from "@fortawesome/free-solid-svg-icons";

import { resetFeedback, sendFeedback, askFeedback } from "actions/counter";

class Typeform extends Component {
    constructor(props: any) {
        super(props);
        this.state = { feedback: "", step: 1 };
    }

    componentDidMount() {
        this.setState({ step: 1 });
    }

    componentDidUpdate(prevProps) {
        if (this.props.resetFeedback) {
            this.setState({ feedback: "" });
            this.props.dispatch(resetFeedback(false));
        }
    }

    UpdateFeedback = (evt: any) => {
        this.setState(
            {
                feedback: evt.target.value,
            },
            function () {
                console.log(this.state.feedback);
            }
        );
    };

    SubmitFeedback = (evt, feedback_type) => {
        if (this.state.feedback !== "") {
            if (evt.key === "Enter") {
                const os = require("os");
                this.setState({ step: 1 });
                this.props.dispatch(
                    sendFeedback(
                        "User OS: " +
                            os.platform() +
                            "<p></p> User Feedback: " +
                            this.state.feedback,
                        feedback_type
                    )
                );
                this.props.dispatch(askFeedback(false));
            }
        } else if (this.state.feedback === "" && evt.key === "Enter") {
            this.setState({ step: 1 });
            this.props.dispatch(askFeedback(false));
        }
    };

    ForwardFeedbackButton = () => {
        this.setState({ step: 1 });
        this.props.dispatch(askFeedback(false));
    };

    SubmitFeedbackButton = (feedback_type) => {
        this.setState({ step: 1 });

        this.props.dispatch(askFeedback(false));
        if (this.state.feedback) {
            const os = require("os");
            this.props.dispatch(
                sendFeedback(
                    "User OS: " +
                        os.platform() +
                        "<p></p> User Feedback: " +
                        this.state.feedback,
                    feedback_type
                )
            );
        }
    };

    NextStep = (evt) => {
        console.log(evt.key);
        if (evt.key === "A") {
            this.setState({ step: 2.1 });
        } else if (evt.key === "B") {
            this.setState({ step: 2.2 });
        } else if (evt.eky === "C") {
            this.props.dispatch(askFeedback(false));
        }
    };

    render() {
        if (this.state.step === 1 && this.props.askFeedback) {
            return (
                <div
                    onKeyDown={this.NextStep}
                    style={{
                        position: "absolute",
                        top: 0,
                        left: 0,
                        width: 900,
                        height: 600,
                        background: "#EEEEEE",
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
                                fontSize: 26,
                                color: "#0980b0",
                                fontWeight: "bold",
                            }}
                        >
                            Thanks for using Fractal! How was it?
                        </div>
                        <div style={{ marginTop: 40 }}>
                            <button
                                className={styles.typeformButton}
                                onClick={() => this.setState({ step: 2.1 })}
                            >
                                <div className={styles.key}>A</div>
                                <div className={styles.text}>
                                    I experienced an issue / bug
                                </div>
                            </button>
                            <button
                                className={styles.typeformButton}
                                onClick={() => this.setState({ step: 2.2 })}
                            >
                                <div className={styles.key}>B</div>
                                <div className={styles.text}>
                                    I'd like to provide feedback
                                </div>
                            </button>
                            <button className={styles.typeformButton}>
                                <div className={styles.key}>C</div>
                                <div
                                    className={styles.text}
                                    onClick={this.ForwardFeedbackButton}
                                >
                                    Great! Continue to dashboard
                                </div>
                            </button>
                        </div>
                    </div>
                </div>
            );
        } else if (this.state.step === 2.1 && this.props.askFeedback) {
            return (
                <div
                    tabIndex="0"
                    onKeyDown={(evt) => this.SubmitFeedback(evt, "bug")}
                    style={{
                        position: "absolute",
                        top: 0,
                        left: 0,
                        width: 900,
                        height: 600,
                        background: "#EEEEEE",
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
                                color: "#0980b0",
                                fontWeight: "bold",
                            }}
                        >
                            We're sorry that you experienced a bug.
                        </div>
                        <div
                            style={{
                                marginTop: 20,
                                fontSize: 15,
                                color: "#0980b0",
                                opacity: 0.6,
                                lineHeight: 1.4,
                            }}
                        >
                            Our engineers rely on the bugs that you report to
                            build a better product, and will address your issue
                            promptly.
                        </div>
                        <input
                            type="text"
                            value={this.state.feedback}
                            placeholder="Please be as descriptive as possible!"
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
                                color: "#0980b0",
                            }}
                        ></input>
                        {this.state.feedback === "" ? (
                            <div>
                                <button
                                    onClick={this.ForwardFeedbackButton}
                                    className={styles.feedbackButton}
                                    style={{
                                        display: "inline",
                                        width: 110,
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
                                        color: "#0980b0",
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
                                    onClick={() =>
                                        this.SubmitFeedbackButton("bug")
                                    }
                                    className={styles.feedbackButton}
                                    style={{
                                        display: "inline",
                                        width: 110,
                                    }}
                                >
                                    SUBMIT
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
                                        color: "#0980b0",
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
            );
        } else if (this.state.step === 2.2 && this.props.askFeedback) {
            return (
                <div
                    tabIndex="0"
                    onKeyDown={(evt) => this.SubmitFeedback(evt, "feedback")}
                    style={{
                        position: "absolute",
                        top: 0,
                        left: 0,
                        width: 900,
                        height: 600,
                        background: "#EEEEEE",
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
                                color: "#0980b0",
                                fontWeight: "bold",
                            }}
                        >
                            How Was Your Experience?
                        </div>
                        <div
                            style={{
                                marginTop: 20,
                                fontSize: 15,
                                color: "#0980b0",
                                opacity: 0.6,
                                lineHeight: 1.4,
                            }}
                        >
                            From feature suggestions, general feedback, to
                            questions and concerns, we'd love to hear from you
                            and will respond promptly.
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
                                color: "#0980b0",
                            }}
                        ></input>
                        {this.state.feedback === "" ? (
                            <div>
                                <button
                                    onClick={this.ForwardFeedbackButton}
                                    className={styles.feedbackButton}
                                    style={{
                                        display: "inline",
                                        width: 110,
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
                                        color: "#0980b0",
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
                                    onClick={() =>
                                        this.SubmitFeedbackButton("feedback")
                                    }
                                    className={styles.feedbackButton}
                                    style={{
                                        display: "inline",
                                        width: 110,
                                    }}
                                >
                                    SUBMIT
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
                                        color: "#0980b0",
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
            );
        } else {
            return <div></div>;
        }
    }
}

function mapStateToProps(state: any) {
    return {
        askFeedback: state.counter.askFeedback,
        resetFeedback: state.counter.resetFeedback,
    };
}

export default connect(mapStateToProps)(Typeform);
