import React, { useState, Dispatch } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { FaChevronLeft, FaChevronRight } from "react-icons/fa"

import history from "shared/utils/history"

import { PriceBox } from "pages/dashboard/settings/payment/plan/components/priceBox"
import { PLANS } from "shared/constants/stripe"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { FractalPlan } from "shared/types/payment"
import { PaymentFlow } from "store/reducers/dashboard/default"
import { User } from "store/reducers/auth/default"
import { PLAN_IDS } from "testing/utils/testIDs"

const Plan = (props: {
    dispatch: Dispatch<any>
    user: User
    paymentFlow: PaymentFlow
}) => {
    /*
        Component that lets users select a plan.
 
        Arguments:
            user (User): User from Redux state
            paymentFlow (PaymentFlow): PaymentFlow from Redux state
            dispatch (Dispatch<any>): Action dispatcher
    */

    const { dispatch, user, paymentFlow } = props
    const [checkedPlan, setCheckedPlan] = useState(
        paymentFlow.plan ? paymentFlow.plan : "Starter"
    )

    const validUser = user.userID && user.userID !== ""

    const submitPlan = () => {
        dispatch(PaymentPureAction.updatePaymentFlow({ plan: checkedPlan }))
        history.push("/dashboard/settings/payment/billing")
    }

    const goBack = () => {
        history.goBack()
    }

    if (!validUser) {
        return <Redirect to="/auth" />
    } else {
        return (
            <div className="relative m-auto md:pt-8 max-w-screen-md">
                <div className="flex cursor-pointer" onClick={goBack}>
                    <FaChevronLeft className="relative top-1.5 mr-2 text-xs" />
                    Back
                </div>
                <div data-testid={PLAN_IDS.INFO}>
                    <div className="mt-4 text-4xl font-medium">
                        Confirm Your Plan
                    </div>
                    <div className="mt-6">
                        Currently, only the Starter plan is available. More
                        plans will be coming soon.
                    </div>
                </div>
                <div data-testid={PLAN_IDS.BOXES}>
                    <Row className="mt-10">
                        <Col
                            md={5}
                            onClick={() => setCheckedPlan(FractalPlan.STARTER)}
                            className=""
                        >
                            <PriceBox
                                {...PLANS[FractalPlan.STARTER]}
                                checked={true}
                            />
                        </Col>
                        <Col
                            md={5}
                            // onClick={() => setCheckedPlan("Pro")}
                            className=""
                        >
                            <PriceBox
                                {...PLANS[FractalPlan.PRO]}
                                checked={false}
                                disabled={true}
                            />
                        </Col>
                    </Row>
                </div>
                <div data-testid={PLAN_IDS.NEXT}>
                    <button
                        className="mt-8 text-md font-medium flex"
                        style={{ opacity: checkedPlan ? "1" : ".6" }}
                        disabled={!checkedPlan}
                        onClick={submitPlan}
                    >
                        Continue to Payment
                        <FaChevronRight className="relative top-1.5 ml-2 text-xs" />
                    </button>
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User }
    DashboardReducer: { paymentFlow: PaymentFlow }
}) => {
    return {
        user: state.AuthReducer.user,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Plan)
