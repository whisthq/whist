import { DEFAULT } from "store/reducers/dashboard/default"

import * as PaymentPureAction from "store/actions/dashboard/payment/pure"

import { deep_copy } from "shared/utils/reducerHelpers"

const DashboardReducer = (state = DEFAULT, action: any) => {
    const stateCopy: any = deep_copy(state)
    switch (action.type) {
        case PaymentPureAction.UPDATE_STRIPE_INFO:
            return {
                ...stateCopy,
                stripeInfo: Object.assign(stateCopy.stripeInfo, action.body),
            }
        case PaymentPureAction.UPDATE_PAYMENT_FLOW:
            return {
                ...stateCopy,
                paymentFlow: Object.assign(stateCopy.paymentFlow, action.body),
            }
        default:
            return state
    }
}

export default DashboardReducer
