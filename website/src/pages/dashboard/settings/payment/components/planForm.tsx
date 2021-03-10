import React from "react"
import { connect } from "react-redux"

import history from "shared/utils/history"
import { PLANS } from "shared/constants/stripe"
import { StripeInfo } from "shared/types/reducers"
import classNames from "classnames"

import styles from "styles/profile.module.css"
import { E2E_DASHBOARD_IDS, PLAN_IDS } from "testing/utils/testIDs"

const PlanForm = (props: { stripeInfo: StripeInfo }) => {
    /*
        Component that redirects users to subscribe to a plan if they are not subscribed
        or shows them their current subscription.
 
        Arguments:
            stripeInfo (StripeInfo): StripeInfo from Redux state
    */

    const { stripeInfo } = props

    const cancelPlan = () => {
        history.push("/dashboard/settings/payment/cancel")
    }

    const addPlan = () => {
        history.push("/dashboard/settings/payment/plan")
    }

    // const changePlan = () => {
    //     history.push("/dashboard/settings/payment/plan")
    // }

    return (
        <div style={{ marginBottom: 25 }}>
            <div className="flex justify-between">
                <div className={classNames("font-body", styles.sectionTitle)}>Plan</div>
                <div
                    style={{
                        display: "flex",
                    }}
                >
                    {stripeInfo.plan && (
                        <button
                            type="button"
                            onClick={cancelPlan}
                            className="font-body text-red cursor-pointer text-sm font-medium bg-none outline-none border-none pb-3"
                            id={E2E_DASHBOARD_IDS.CANCELPLAN}
                        >
                            Cancel
                        </button>
                    )}
                    {/* <div className={styles.change} onClick={changePlan}>
                        Change
                    </div> */}
                </div>
            </div>
            <div className={styles.sectionInfo}>
                {stripeInfo.plan && PLANS[stripeInfo.plan] ? (
                    <div
                        className={styles.dashedBox}
                        data-testid={PLAN_IDS.ADDPLAN}
                        style={{
                            display: "flex",
                            justifyContent: "space-between",
                        }}
                    >
                        <div className="font-body font-medium">{stripeInfo.plan}</div>
                        <div className="font-body">
                            <span
                                className="font-body relative bottom-0.5 right-1 font-medium"
                                style={{
                                    fontSize: 10,
                                }}
                            >
                                $
                            </span>
                            {PLANS[stripeInfo.plan].price}{" "}
                            <span
                                style={{ fontSize: 10 }}
                                className="font-medium"
                            >
                                / mo
                            </span>
                        </div>
                    </div>
                ) : (
                    <div
                        id={E2E_DASHBOARD_IDS.ADDPLAN}
                        className={styles.dashedBox}
                        data-testid={PLAN_IDS.ADDPLAN}
                        onClick={addPlan}
                        style={{ cursor: "pointer" }}
                    >
                        + Add a plan
                    </div>
                )}
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    DashboardReducer: { stripeInfo: StripeInfo }
}) => {
    return {
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(PlanForm)
