import React, { useState } from "react"
import { connect } from "react-redux"
import { Alert } from "react-bootstrap"
import TextareaAutosize from "react-textarea-autosize"
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

    const updateType = (type: string) => {
        setType(type)
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
            <h2>Support</h2>
            <div style={{ marginTop: 30, fontSize: 15 }}>
                If you have any feedback to make Fractal better, or need to
                contact our team, please fill out the form below, and we'll get
                back to you as soon as possible!
            </div>
            <div style={{ marginTop: 40, fontSize: 15 }}>
                This best falls under the category:
            </div>
            <form
                style={{
                    display: "flex",
                    flexDirection: "row",
                    justifyContent: "space-between",
                    marginTop: 15,
                    fontSize: 14,
                    width: 500,
                }}
            >
                <div>
                    <input
                        type="radio"
                        checked={type === "Bug"}
                        onChange={() => updateType("Bug")}
                    />
                    <label style={{ padding: 5, marginLeft: 7 }}>Bug</label>
                </div>
                <div>
                    <input
                        type="radio"
                        checked={type === "Question"}
                        onChange={() => updateType("Question")}
                    />
                    <label style={{ padding: 5, marginLeft: 7 }}>
                        Question
                    </label>
                </div>
                <div>
                    <input
                        type="radio"
                        checked={type === "Feedback"}
                        onChange={() => updateType("Feedback")}
                    />
                    <label style={{ padding: 5, marginLeft: 7 }}>
                        General Feedback
                    </label>
                </div>
            </form>
            <TextareaAutosize
                value={feedback}
                placeholder="Your feedback here!"
                onChange={updateFeedback}
                style={{
                    marginTop: 20,
                    width: "100%",
                    background: "none",
                    border: "none",
                    borderBottom: "solid 0.5px #555555",
                    outline: "none",
                    padding: "10px 10px 10px 0px",
                    fontSize: 14,
                    color: "#555555",
                }}
            />
            {showSubmittedAlert && (
                <Alert
                    variant="success"
                    onClose={() => setShowSubmittedAlert(false)}
                    dismissible
                    style={{ marginTop: 15, fontSize: 14 }}
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
