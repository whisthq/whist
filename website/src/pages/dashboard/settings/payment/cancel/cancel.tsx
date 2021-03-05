import React, { useState, useEffect, Dispatch } from "react"
import { connect } from "react-redux"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"
import { Redirect } from "react-router"
import { StripeInfo } from "store/reducers/dashboard/default"
import { User } from "store/reducers/auth/default"

import history from "shared/utils/history"

import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

import { CANCEL_IDS } from "testing/utils/testIDs"

const Cancel = (props: {
    dispatch: Dispatch<any>
    user: User
    stripeInfo: StripeInfo
}) => {
    /*
        Form that asks a user why they are cancelling their plan and allows them to confirm
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            stripeInfo (StripeInfo): StripeInfo from Redux state
            user (User): User from Redux state
            editingCard (boolean): true/false whether the user is editing their card info
    */

    const { user, dispatch, stripeInfo } = props

    const [feedback, setFeedback] = useState("")
    const [submitting, setSubmitting] = useState(false)
    const [warning, setWarning] = useState(false)

    const validUser = user.userID && user.userID !== ""

    useEffect(() => {
        if (stripeInfo.stripeRequestReceived) {
            setSubmitting(false)
            if (stripeInfo.stripeStatus === "success") {
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        plan: null,
                        stripeRequestReceived: false,
                        stripeStatus: null,
                    })
                )
                history.push("/dashboard/settings/payment/canceled")
            } else if (stripeInfo.stripeStatus === "failure") {
                setWarning(true)
                dispatch(
                    PaymentPureAction.updateStripeInfo({
                        stripeRequestReceived: false,
                        stripeStatus: null,
                    })
                )
            }
        }
    }, [dispatch, stripeInfo])

    const updateFeedback = (evt: any) => {
        setFeedback(evt.target.value)
    }

    const cancelPlan = () => {
        setSubmitting(true)
        dispatch(PaymentSideEffect.deleteSubscription(feedback))
    }

    const backToProfile = () => {
        history.push("/dashboard/settings/payment")
    }
    if (!validUser) {
        return <Redirect to="/auth" />
    } else if (!stripeInfo.plan) {
        return <Redirect to="/dashboard" />
    } else {
        return (
            <div
                className="relative m-auto pt-8 max-w-screen-md"
                data-testid={CANCEL_IDS.INFO}
            >
                <div className="mt-4 text-4xl font-medium tracking-wide">
                    Cancel Plan
                </div>
                <div className="mt-6">
                    We're always striving to improve Fractal. Could you briefly
                    describe why you are canceling your plan or provide any
                    feedback that could help us improve?
                </div>
                <div data-testid={CANCEL_IDS.FEEDBACK}>
                    <textarea
                        value={feedback}
                        placeholder="Your feedback here!"
                        onChange={updateFeedback}
                        wrap="soft"
                        className="w-full mt-4 p-6 resize-none border-0 outline-none bg-blue-lightest rounded"
                    />
                </div>
                {warning && (
                    <div className="mt-4">
                        There was a problem canceling your plan. Please try
                        again.
                    </div>
                )}
                <div className="flex">
                    <div data-testid={CANCEL_IDS.SUBMIT}>
                        <button
                            className="px-12 py-2.5 bg-blue text-white mt-4 rounded mr-2 hover:bg-mint hover:text-black duration-500"
                            onClick={cancelPlan}
                        >
                            {submitting ? (
                                <FontAwesomeIcon
                                    icon={faCircleNotch}
                                    spin
                                    style={{
                                        color: "white",
                                    }}
                                />
                            ) : (
                                <span>Continue</span>
                            )}
                        </button>
                    </div>
                    <div data-testid={CANCEL_IDS.BACK}>
                        <button
                            className="px-12 py-2.5 text-gray mt-4"
                            onClick={backToProfile}
                        >
                            Back
                        </button>
                    </div>
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User }
    DashboardReducer: { stripeInfo: StripeInfo }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(Cancel)
