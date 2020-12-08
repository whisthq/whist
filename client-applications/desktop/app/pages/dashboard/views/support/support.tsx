import React, {
    useState,
    useEffect,
    ChangeEvent,
    KeyboardEvent,
    Dispatch,
} from "react"
import { connect } from "react-redux"
import { Row } from "react-bootstrap"

import { submitFeedback } from "store/actions/sideEffects"
import FractalKey from "shared/types/input"

import styles from "pages/dashboard/views/support/support.css"
import dashboardStyles from "pages/dashboard/dashboard.css"

const Support = (props: { dispatch: Dispatch<any> }) => {
    const { dispatch } = props

    const [feedback, setFeedback] = useState("")
    const [type, setType] = useState("")
    const [showSubmittedAlert, setShowSubmittedAlert] = useState(false)

    const updateFeedback = (evt: ChangeEvent<HTMLTextAreaElement>) => {
        setFeedback(evt.target.value)
    }

    const handleSelect = (evt: ChangeEvent<HTMLSelectElement>) => {
        setType(evt.target.value)
    }

    const handleSubmit = () => {
        if (feedback !== "" && type !== "") {
            dispatch(submitFeedback(feedback, type))
            setShowSubmittedAlert(true)
            setFeedback("")
            setType("")
        }
    }

    useEffect(() => {
        if (showSubmittedAlert) {
            setTimeout(() => {
                setShowSubmittedAlert(false)
            }, 5000)
        }
    }, [showSubmittedAlert])

    return (
        <div className={dashboardStyles.page}>
            <Row>
                <div style={{ marginTop: 30, fontSize: 15 }}>
                    If you have any feedback to make Fractal better, or need to
                    contact our team, please fill out the form below, and
                    we&lsquo;ll get back to you as soon as possible!
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
                {(type && feedback) || showSubmittedAlert ? (
                    <div
                        role="button"
                        tabIndex={0}
                        onKeyDown={(event: KeyboardEvent<HTMLDivElement>) => {
                            if (event.key === FractalKey.ENTER) {
                                handleSubmit()
                            }
                        }}
                        className={
                            showSubmittedAlert
                                ? dashboardStyles.submittedButton
                                : dashboardStyles.feedbackButton
                        }
                        onClick={handleSubmit}
                    >
                        {showSubmittedAlert ? "Submitted!" : "Submit"}
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

const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(Support)
