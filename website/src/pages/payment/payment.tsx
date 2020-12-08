import React, { useState } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import PaymentMethod from "pages/payment/components/paymentMethod"
import Checkout from "pages/payment/components/checkout"

import { config } from "shared/constants/config"

import "styles/payment.css"

const Payment = (props: {
    dispatch: any
    user: any
    stripeInfo: any
    paymentFlow: { plan: string }
}) => {
    const { user, stripeInfo, paymentFlow } = props

    const [editingCard, setEditingCard] = useState(
        !(stripeInfo.cardBrand && stripeInfo.cardLastFour)
    )

    const valid_user = user.user_id && user.user_id !== ""

    if (!valid_user) {
        return <Redirect to="/auth/bypass" />
    } else if (!paymentFlow.plan) {
        return <Redirect to="/plan" />
    } else if (!config.payment_enabled) {
        return <Redirect to="/profile" />
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

const mapStateToProps = (state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any; paymentFlow: any }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
        paymentFlow: state.DashboardReducer.paymentFlow,
    }
}

export default connect(mapStateToProps)(Payment)
