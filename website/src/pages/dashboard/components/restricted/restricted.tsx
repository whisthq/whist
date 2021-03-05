import React, { useState } from "react"
import { connect } from "react-redux"

import { User } from "store/reducers/auth/default"
import Typeform from "pages/dashboard/components/restricted/components/typeform"

const Restricted = (props: { user: User; formFilledOut: boolean }) => {
    /*
        Page that prevents un-invited user from seeing the dashboard

        Arguments:
            user (User): Current logged in user
            formFilledOut (boolean): True/false if user has submitted the onboarding Typeform
    */

    const { user, formFilledOut } = props
    const [showForm, setShowForm] = useState(false)

    const handleClose = () => {
        setShowForm(false)
    }

    if (!formFilledOut) {
        return (
            <div className="h-screen md:h-96">
                <div className="m-auto max-w-screen-sm text-center bg-blue-lightest rounded px-12 py-12 border border-gray-400 relative top-16">
                    <div className="text-3xl">An invite is required.</div>
                    <div className="mt-4">
                        Fractal is currently in beta and access is invite-only.
                        You can apply for an invite below. Afterward, you'll be
                        added to our invite list and we'll email you when we're
                        ready to have you on board.
                    </div>
                    <button
                        type="button"
                        className="rounded bg-blue text-white border-none px-10 py-3 mt-8 duration-500 hover:bg-mint hover:text-gray"
                        onClick={() => setShowForm(true)}
                    >
                        APPLY NOW
                    </button>
                    <div
                        className="h-96"
                        style={{ display: showForm ? "block" : "none" }}
                    >
                        <Typeform
                            show={showForm}
                            handleClose={handleClose}
                        />
                    </div>
                </div>
            </div>
        )
    } else {
        return (
            <div className="h-screen md:h-96">
                <div className="m-auto max-w-screen-sm text-center bg-blue-lightest rounded px-12 py-12 border border-gray-400 relative top-16">
                    <div className="text-3xl">An invite on its way.</div>
                    <div className="mt-4">
                        Thank you for applying for an invite. We're currently
                        sending out invites on a rolling basis. You should
                        expect to receive one via email at {user.userID} within
                        the next few weeks!
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

export default connect(mapStateToProps)(Restricted)
