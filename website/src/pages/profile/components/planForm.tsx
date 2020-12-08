import React from "react"
import { connect } from "react-redux"

import history from "shared/utils/history"

import { PLANS } from "shared/constants/stripe"

import "styles/profile.css"

const PlanForm = (props: any) => {
    const { stripeInfo } = props

    const cancelPlan = () => {
        history.push("/cancel")
    }

    const addPlan = () => {
        history.push("/plan")
    }

    return (
        <>
            <div className="section-title">Plan</div>
            <div className="section-info">
                {stripeInfo.plan && PLANS[stripeInfo.plan] ? (
                    <>
                        <div>
                            {stripeInfo.plan} - $
                            {PLANS[stripeInfo.plan].price.toFixed(2)} /mo (
                            {PLANS[stripeInfo.plan].subtext})
                        </div>
                        <div className="add" onClick={cancelPlan}>
                            Cancel
                        </div>
                    </>
                ) : (
                    <div className="add" onClick={addPlan}>
                        + Add a plan
                    </div>
                )}
            </div>
        </>
    )
}

function mapStateToProps(state: { DashboardReducer: { stripeInfo: any } }) {
    return {
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(PlanForm)
