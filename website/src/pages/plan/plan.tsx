import React, { useState } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import history from "shared/utils/history"

import Header from "shared/components/header"
import PriceBox from "pages/plan/components/priceBox"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"

import "styles/plan.css"

const Plan = (props: any) => {
    const { dispatch, user, paymentFlow } = props
    const [checkedPlan, setCheckedPlan] = useState(
        paymentFlow.plan ? paymentFlow.plan : ""
    )

    const valid_user = user.user_id && user.user_id !== ""

    const submitPlan = () => {
        dispatch(PaymentPureAction.updatePaymentFlow({ plan: checkedPlan }))
        history.push("/payment")
    }

    if (!valid_user) {
        return <Redirect to="/auth/bypass" />
    } else {
        return (
            <div className="fractalContainer">
                <Header dark={false} account />
                <div className="plan">
                    <h3>Choose your plan</h3>
                    <div>
                        No payment is required until your free trial has ended.
                        You can change or cancel your plan at any time.
                    </div>
                    <div className="priceBoxes">
                        <div
                            onClick={() => setCheckedPlan("Hourly")}
                            className="priceBox-wrapper"
                        >
                            <PriceBox
                                name="Hourly"
                                subText="Starts with 7-day free trial"
                                price="5"
                                details="+$0.70 / hr of usage"
                                checked={checkedPlan === "Hourly"}
                            />
                        </div>
                        <div
                            onClick={() => setCheckedPlan("Monthly")}
                            className="priceBox-wrapper"
                        >
                            <PriceBox
                                name="Monthly"
                                subText="Starts with 7-day free trial"
                                price="39"
                                details={
                                    <div>
                                        6 hr / day
                                        <br />
                                        +$0.50 per extra hr
                                    </div>
                                }
                                checked={checkedPlan === "Monthly"}
                            />
                        </div>
                        <div
                            onClick={() => setCheckedPlan("Unlimited")}
                            className="priceBox-wrapper"
                        >
                            <PriceBox
                                name="Unlimited"
                                subText="Starts with 7-day free trial"
                                price="99"
                                details="Unlimited daily usage"
                                checked={checkedPlan === "Unlimited"}
                            />
                        </div>
                    </div>
                    <button
                        className="next-button"
                        style={{ opacity: checkedPlan ? "1" : ".6" }}
                        disabled={!checkedPlan}
                        onClick={submitPlan}
                    >
                        Next: Payment
                    </button>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: {
    AuthReducer: { user: any }
    DashboardReducer: { paymentFlow: any }
}) {
    return {
        user: state.AuthReducer.user,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Plan)
