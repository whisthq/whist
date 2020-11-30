import React, { useState } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import PaymentMethod from "pages/payment/components/paymentMethod"
import Checkout from "pages/payment/components/checkout"

import "styles/payment.css"

const Payment = (props: {
    dispatch: any
    user: any
    paymentFlow: { plan: string }
}) => {
    const { user, paymentFlow } = props

    const [editingCard, setEditingCard] = useState(
        !(user.cardBrand && user.cardLastFour)
    )

    const valid_user = user.user_id && user.user_id !== ""

    if (!valid_user) {
        return <Redirect to="/auth/bypass" />
    } else if (!paymentFlow.plan) {
        return <Redirect to="/plan" />
    } else {
        return (
            <div className="fractalContainer">
                <Header dark={false} account />
                <div className="payment-container">
                    <PaymentMethod
                        editingCard={editingCard}
                        setEditingCard={setEditingCard}
                    />
                    <Checkout editingCard={editingCard} />
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any; paymentFlow: any }
}) {
    return {
        user: state.AuthReducer.user,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Payment)
