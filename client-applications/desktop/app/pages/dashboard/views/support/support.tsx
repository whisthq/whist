import React, { useState, ChangeEvent } from "react"
import { connect } from "react-redux"
import { Alert, Row } from "react-bootstrap"

import { submitFeedback } from "store/actions/sideEffects"

import styles from "pages/dashboard/views/support/support.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

const Settings = <T extends {}>(props: T) => {
    const { dispatch } = props

    const [feedback, setFeedback] = useState("")
    const [type, setType] = useState("")
    const [showSubmittedAlert, setShowSubmittedAlert] = useState(false)

    const updateFeedback = (evt: ChangeEvent) => {
        setFeedback(evt.target.value)
    }

    const handleSelect = (evt: ChangeEvent) => {
        setType(evt.target.value)
    }

    const handleSubmit = () => {
        dispatch(submitFeedback(feedback, type))
        setShowSubmittedAlert(true)
        setFeedback("")
        setType("")
    }

    return (
        <div className={dashboardStyles.page}>
            <Row>
                <div style={{ marginTop: 30, fontSize: 15 }}>
                    If you have any feedback to make Fractal better, or need to
                    contact our team, please fill out the form below, and we'll
                    get back to you as soon as possible!
                </div>
                <form className={styles.form}>
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
                    className={styles.textBox}
                />
                {showSubmittedAlert && (
                    <Alert
                        variant="success"
                        onClose={() => setShowSubmittedAlert(false)}
                        dismissible
                        className={styles.alert}
                    >
                        Your feedback has been submitted!
                    </Alert>
                )}
                {type && feedback ? (
                    <div
                        className={dashboardStyles.feedbackButton}
                        onClick={handleSubmit}
                    >
                        Submit
                    </div>
                ) : (
                    <div className={dashboardStyles.noFeedbackButton}>
                        Submit
                    </div>
                )}
            </Row>
        </div>
    )
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {}
}

export default connect(mapStateToProps)(Settings)
