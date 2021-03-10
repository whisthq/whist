import React from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import history from "shared/utils/history"
import { User } from "shared/types/reducers"
import { CANCELED_IDS } from "testing/utils/testIDs"

const Canceled = (props: { user: User }) => {
    /*
        After cancelling their plan, users are shown this page
 
        Arguments:
            user (User): User from Redux state            
    */

    const { user } = props

    const validUser = user.userID && user.userID !== ""

    const addPlan = () => {
        history.push("/dashboard/settings/payment/plan")
    }

    const backToProfile = () => {
        history.push("/dashboard/settings")
    }

    if (!validUser) {
        return <Redirect to="/auth" />
    } else {
        return (
            <div className="relative m-auto pt-8 max-w-screen-sm">
                <div data-testid={CANCELED_IDS.INFO}>
                    <div className="mt-4 text-4xl font-medium tracking-wide">
                        Successfully cancelled plan.
                    </div>
                </div>
                <div className="flex-none md:flex mt-4">
                    <div data-testid={CANCELED_IDS.ADD}>
                        <button
                            className="font-body px-12 py-2.5 bg-blue text-white mt-4 rounded mr-2 hover:bg-mint hover:text-black duration-500"
                            onClick={addPlan}
                        >
                            Add a New Plan
                        </button>
                    </div>
                    <div data-testid={CANCELED_IDS.BACK}>
                        <button
                            className="font-body md:px-12 py-2.5 text-gray mt-4"
                            onClick={backToProfile}
                        >
                            Back to Profile
                        </button>
                    </div>
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: { AuthReducer: { user: User } }) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Canceled)
