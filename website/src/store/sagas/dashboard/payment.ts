import { put, takeEvery, all, call, select } from "redux-saga/effects"

import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

import * as api from "shared/api"
// note that the tokens here are usually all stripe tokens
// not jwt tokens (i.e. the type you get when you create a token from
// a card number + date + digits on the back)

export function* addCard(action: any): any {
    const state = yield select()

    const { json } = yield call(
        api.paymentCardAdd,
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken,
        state.AuthReducer.user.userID,
        action.source
    )

    if (json) {
        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestReceived: true,
                    stripeStatus: "success",
                    cardBrand: action.source.card.brand,
                    cardLastFour: action.source.card.last4,
                    postalCode: action.source.owner.address.postal_code,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestReceived: true,
                    stripeStatus: "failure",
                })
            )
        }
    } else {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestReceived: true,
                stripeStatus: "failure",
            })
        )
    }
}

export function* deleteCard(action: any): any {
    const state = yield select()

    const { json } = yield call(
        api.paymentCardDelete,
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken,
        state.AuthReducer.user.userID,
        action.source
    )

    if (json) {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestReceived: true,
            })
        )

        if (json.status === 200) {
            PaymentPureAction.updateStripeInfo({
                cardBrand: null,
                cardLastFour: null,
                postalCode: null,
            })
        }
    }
}

// to modify subscription just call this
// or pass in a null token (not yet supported)
export function* addSubscription(action: any): any {
    const state = yield select()

    const { json } = yield call(
        api.addSubscription,
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken,
        state.AuthReducer.user.userID,
        action.plan
    )

    if (json) {
        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestReceived: true,
                    checkoutStatus: "success",
                    plan: action.plan,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestReceived: true,
                    checkoutStatus: "failure",
                })
            )
        }
    } else {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestReceived: true,
                checkoutStatus: "failure",
            })
        )
    }
}

export function* deleteSubscription(action: any): any {
    const state = yield select()

    yield call(api.cancelMail, state.AuthReducer.user.userID, action.message)

    const { json } = yield call(
        api.deleteSubscription,
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken,
        state.AuthReducer.user.userID
    )

    if (json) {
        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestReceived: true,
                    stripeStatus: "success",
                    plan: null,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestReceived: true,
                    stripeStatus: "failure",
                })
            )
        }
    } else {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestReceived: true,
                stripeStatus: "failure",
            })
        )
    }
}

export default function* paymentSaga() {
    yield all([
        takeEvery(PaymentSideEffect.ADD_CARD, addCard),
        takeEvery(PaymentSideEffect.DELETE_CARD, deleteCard),
        takeEvery(PaymentSideEffect.ADD_SUBSCRIPTION, addSubscription),
        takeEvery(PaymentSideEffect.DELETE_SUBSCRIPTION, deleteSubscription),
    ])
}
