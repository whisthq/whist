import React, { useState, Dispatch } from "react"
import { connect } from "react-redux"
import { FaSignOutAlt } from "react-icons/fa"
import { Modal } from "react-bootstrap"

import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT as AUTH_DEFAULT } from "store/reducers/auth/default"
import { DEFAULT as DASHBOARD_DEFAULT } from "store/reducers/dashboard/default"

export const SignoutButton = (props: {
    dispatch: Dispatch<any>
    id?: string
}) => {
    /*
        Signout button on the left-hand side of the dashboard
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher  
    */

    const { id, dispatch } = props
    const [showModal, setShowModal] = useState(false)

    const handleSignOut = () => {
        dispatch(PureAuthAction.updateUser(deepCopy(AUTH_DEFAULT.user)))
        dispatch(PureAuthAction.updateAuthFlow(deepCopy(AUTH_DEFAULT.authFlow)))
        dispatch(
            PaymentPureAction.updateStripeInfo(
                deepCopy(DASHBOARD_DEFAULT.stripeInfo)
            )
        )
        dispatch(
            PaymentPureAction.updatePaymentFlow(
                deepCopy(DASHBOARD_DEFAULT.paymentFlow)
            )
        )
    }

    return (
        <>
            <div
                id={id}
                className="w-12 h-12 mb-8 rounded relative bg-blue-100 shadow-xl cursor-pointer"
                onClick={() => setShowModal(!showModal)}
            >
                <FaSignOutAlt className="absolute top-1/2 left-1/2 transform -translate-y-1/2 -translate-x-1/2 text-blue-300 text-lg" />
            </div>
            <Modal
                show={showModal}
                onHide={() => setShowModal(false)}
                style={{ border: "none" }}
                size="sm"
            >
                <Modal.Body className="text-center py-6 px-10">
                    <div className="font-body">Are you sure you want to sign out?</div>
                    <button
                        className="font-body bg-red hover:bg-red-800 duration-500 text-white w-full rounded mt-6 border-none outline-none py-2"
                        onClick={handleSignOut}
                    >
                        Sign Out
                    </button>
                </Modal.Body>
            </Modal>
        </>
    )
}

export default connect()(SignoutButton)
