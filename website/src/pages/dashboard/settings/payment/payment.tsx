import React from "react"
import { Switch, Route } from "react-router-dom"

import withTracker from "shared/utils/withTracker"

import PaymentForm from "pages/dashboard/settings/profile/components/paymentForm"
import PlanForm from "pages/dashboard/settings/payment/components/planForm"
import Plan from "pages/dashboard/settings/payment/plan/plan"
import Cancel from "pages/dashboard/settings/payment/cancel/cancel"
import Canceled from "pages/dashboard/settings/payment/cancel/canceled"
import Confirmation from "pages/dashboard/settings/payment/confirmation/confirmation"
import Billing from "pages/dashboard/settings/payment/billing/billing"

export const Payment = () => {
    /*
        Router for the payment page in settings.
 
        Arguments: none
    */

    return (
        <div>
            {window.location.pathname === "/dashboard/settings/payment" && (
                <div className="w-full mt-12 md:m-auto md:max-w-screen-sm md:mt-16">
                    <PlanForm />
                    <PaymentForm />
                </div>
            )}
            <Switch>
                <Route
                    exact
                    path="/dashboard/settings/payment/billing"
                    component={withTracker(Billing)}
                />
                <Route
                    exact
                    path="/dashboard/settings/payment/plan"
                    component={withTracker(Plan)}
                />
                <Route
                    path="/dashboard/settings/payment/confirmation"
                    component={withTracker(Confirmation)}
                />
                <Route
                    path="/dashboard/settings/payment/cancel"
                    component={withTracker(Cancel)}
                />
                <Route
                    path="/dashboard/settings/payment/canceled"
                    component={withTracker(Canceled)}
                />
            </Switch>
        </div>
    )
}

export default Payment
