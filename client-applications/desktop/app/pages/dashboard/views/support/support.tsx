import React, { useState } from "react"
import { connect } from "react-redux"
import { Alert, Row } from "react-bootstrap"

import { submitFeedback } from "store/actions/sideEffects"

import styles from "pages/dashboard/dashboard.css"

const Settings = <T extends {}>(props: T) => {
    const { dispatch } = props

    const [feedback, setFeedback] = useState("")
    const [type, setType] = useState("")
    const [showSubmittedAlert, setShowSubmittedAlert] = useState(false)

    const updateFeedback = (evt: any) => {
        setFeedback(evt.target.value)
    }

    const handleSelect = (evt: any) => {
        setType(evt.target.value)
    }

    const handleSubmit = () => {
        dispatch(submitFeedback(feedback, type))
        setShowSubmittedAlert(true)
        setFeedback("")
        setType("")
    }

    return (
        <div className={styles.page}>
            <Row>
                <div style={{ marginTop: 30, fontSize: 15 }}>
                    If you have any feedback to make Fractal better, or need to
                    contact our team, please fill out the form below, and we'll
                    get back to you as soon as possible!
                </div>
                <form
                    style={{
                        marginTop: 40,
                        fontSize: 14,
                        borderRadius: 5,
                        boxShadow: "0px 4px 10px rgba(0,0,0,0.1)",
                        width: "100%",
                    }}
                >
                    <select
                        required
                        onChange={handleSelect}
                        defaultValue={type}
                        style={{
                            paddingLeft: 5,
                            width: "100%",
                            borderRadius: 5,
                        }}
                    >
                        <option value="">Category</option>
                        <option value="Bug">Bug</option>
                        <option value="Question">Question</option>
                        <option value="General Feedback">
                            General Feedback
                        </option>
                    </select>
                </form>
                <textarea
                    value={feedback}
                    placeholder="Your feedback here!"
                    onChange={updateFeedback}
                    wrap="soft"
                    style={{
                        marginTop: 20,
                        marginBottom: 10,
                        width: "100%",
                        background: "white",
                        border: "none",
                        borderRadius: 5,
                        outline: "none",
                        padding: "20px",
                        fontSize: 14,
                        color: "#111111",
                        height: "160px",
                        resize: "none",
                        boxShadow: "0px 4px 10px rgba(0,0,0,0.1)",
                    }}
                />
                {showSubmittedAlert && (
                    <Alert
                        variant="success"
                        onClose={() => setShowSubmittedAlert(false)}
                        dismissible
                        style={{
                            marginTop: 11,
                            fontSize: 14,
                            marginBottom: 0,
                            borderRadius: 0,
                            border: "none",
                            padding: 20,
                        }}
                    >
                        Your feedback has been submitted!
                    </Alert>
                )}
                {type && feedback ? (
                    <div
                        className={styles.feedbackButton}
                        onClick={handleSubmit}
                    >
                        Submit
                    </div>
                ) : (
                    <div className={styles.noFeedbackButton}>Submit</div>
                )}
            </Row>
        </div>
    )
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {}
}

export default connect(mapStateToProps)(Settings)
