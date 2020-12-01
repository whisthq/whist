import React, { useState } from "react"
import { connect } from "react-redux"
import { Alert } from "react-bootstrap"
import styles from "styles/dashboard.css"

import { submitFeedback } from "store/actions/sideEffects"

const Settings = (props: any) => {
    const { dispatch } = props

    const [feedback, setFeedback] = useState("")
    const [type, setType] = useState("")
    const [showSubmittedAlert, setShowSubmittedAlert] = useState(false)

    const updateFeedback = (evt: any) => {
        setFeedback(evt.target.value)
    }

    const handleSelect = (evt: any) => {
        setType(evt.target.value)
        console.log(type)
    }

    const handleSubmit = () => {
        dispatch(submitFeedback(feedback, type))
        setShowSubmittedAlert(true)
        setFeedback("")
        setType("")
    }

    return (
        <div className={styles.page}>
            <div style={{ marginTop: 30, fontSize: 15 }}>
                If you have any feedback to make Fractal better, or need to
                contact our team, please fill out the form below, and we'll get
                back to you as soon as possible!
            </div>
            <form
                style={{
                    marginTop: 40,
                    fontSize: 14,
                }}
            >
                <select required onChange={handleSelect} defaultValue={type}>
                    <option selected value="">
                        Category
                    </option>
                    <option value="Bug">Bug</option>
                    <option value="Question">Question</option>
                    <option value="General Feedback">General Feedback</option>
                </select>
            </form>
            <textarea
                value={feedback}
                placeholder="Your feedback here!"
                onChange={updateFeedback}
                wrap="soft"
                style={{
                    marginTop: 20,
                    width: "100%",
                    background: "#F2F6FB",
                    border: "none",
                    outline: "none",
                    padding: "20px",
                    fontSize: 14,
                    color: "#black",
                    height: "160px",
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
                <div className={styles.feedbackButton} onClick={handleSubmit}>
                    SUBMIT
                </div>
            ) : (
                <div className={styles.noFeedbackButton}>SUBMIT</div>
            )}
        </div>
    )
}

const mapStateToProps = (state: any) => {
    return {}
}

export default connect(mapStateToProps)(Settings)
