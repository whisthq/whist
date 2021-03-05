import React, { Dispatch, useState } from "react"
import { Row, Col } from "react-bootstrap"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { FaChevronLeft } from "react-icons/fa"

import history from "shared/utils/history"
import PaymentMethod from "pages/dashboard/settings/payment/billing/components/paymentMethod"
import Checkout from "pages/dashboard/settings/payment/billing/components/checkout"
import { PAYMENT_IDS } from "testing/utils/testIDs"
import { StripeInfo } from "store/reducers/dashboard/default"
import { User } from "store/reducers/auth/default"

const Billing = (props: {
    dispatch: Dispatch<any>
    user: User
    stripeInfo: StripeInfo
    paymentFlow: { plan: string }
    testDate?: boolean
}) => {
    /*
        Checkout form that lets user add a card and submit a payment
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            stripeInfo (StripeInfo): StripeInfo from Redux state
            paymentFlow (PaymentFlow): PaymentFlow from Redux state
            editingCard (boolean): true/false whether the user is editing their card info
            setEditingCard (any): Callback function to set whether card is being edited
    */

    const { user, stripeInfo, paymentFlow } = props

    const [editingCard, setEditingCard] = useState(
        !(stripeInfo.cardBrand && stripeInfo.cardLastFour)
    )

    const validUser = user.userID && user.userID !== ""

    const goBack = () => {
        history.goBack()
    }

    if (!validUser) {
        return <Redirect to="/auth" />
    } else if (!paymentFlow.plan) {
        return <Redirect to="/dashboard/settings/payment/plan" />
    } else {
        return (
            <div className="relative m-auto md:pt-8 max-w-screen-md">
                <div className="flex cursor-pointer" onClick={goBack}>
                    <FaChevronLeft className="relative top-1.5 mr-2 text-xs" />
                    Back
                </div>
                <div data-testid={PAYMENT_IDS.PAY}>
                    <Row>
                        <Col md={6}>
                            <PaymentMethod
                                editingCard={editingCard}
                                setEditingCard={setEditingCard}
                            />
                        </Col>
                        <Col md={6} className="mt-12 md:mt-0">
                            <Checkout
                                editingCard={editingCard}
                                testDate={props.testDate}
                            />
                        </Col>
                    </Row>
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User }
    DashboardReducer: { stripeInfo: StripeInfo; paymentFlow: { plan: string } }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Billing)
