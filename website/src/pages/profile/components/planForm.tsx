import React from "react"
import { connect } from "react-redux"

import history from "shared/utils/history"

import "styles/profile.css"

const PlanForm = (props: any) => {
    const { user } = props

    const addPlan = () => {
        history.push("/plan")
    }

    return (
        <>
            <div className="section-title">Plan</div>
            <div className="section-info">
                {user.plan ? (
                    <>
                        <div>{user.plan}</div>
                        <div>View plan details</div>
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

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(PlanForm)
