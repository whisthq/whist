import { put, takeEvery, all, call, select } from "redux-saga/effects"
import { apiPost } from "shared/utils/api"

import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import * as PaymentSideEffect from "store/actions/dashboard/payment/sideEffects"

// note that the tokens here are usually all stripe tokens
// not jwt tokens (i.e. the type you get when you create a token from
// a card number + date + digits on the back)

function* addCard(action: any) {
    const state = yield select()

    const { json } = yield call(
        apiPost,
        "/stripe/addCard",
        {
            email: state.AuthReducer.user.user_id,
            source: action.source,
        },
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken
    )

    if (json) {
        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    stripeStatus: "success",
                    cardBrand: action.source.card.brand,
                    cardLastFour: action.source.card.last4,
                    postalCode: action.source.owner.address.postal_code,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    stripeStatus: "failure",
                })
            )
        }
    } else {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestRecieved: true,
                stripeStatus: "failure",
            })
        )
    }
}

function* deleteCard(action: any) {
    const state = yield select()

    const { json } = yield call(
        apiPost,
        "/stripe/deleteCard",
        {
            email: state.AuthReducer.user.user_id,
            source: action.source,
        },
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken
    )

    if (json) {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestRecieved: true,
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
function* addSubscription(action: any) {
    const state = yield select()

    const { json } = yield call(
        apiPost,
        "/stripe/addSubscription",
        {
            email: state.AuthReducer.user.user_id,
            plan: action.plan,
        },
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken
    )

    if (json) {
        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    checkoutStatus: "success",
                    plan: action.plan,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    checkoutStatus: "failure",
                })
            )
        }
    } else {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestRecieved: true,
                checkoutStatus: "failure",
            })
        )
    }
}

function* deleteSubscription(action: any) {
    const state = yield select()

    yield call(
        apiPost,
        "/mail/cancel", // a bit old, might want to update?
        {
            username: state.AuthReducer.user.user_id,
            feedback: action.message,
        },
        ""
    )

    const { json } = yield call(
        apiPost,
        "/stripe/deleteSubscription",
        {
            email: state.AuthReducer.user.user_id,
        },
        state.AuthReducer.user.accessToken,
        state.AuthReducer.user.refreshToken
    )

    if (json) {
        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    stripeStatus: "success",
                    plan: null,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    stripeStatus: "failure",
                })
            )
        }
    } else {
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestRecieved: true,
                stripeStatus: "failure",
            })
        )
    }
}

export default function* () {
    yield all([
        takeEvery(PaymentSideEffect.ADD_CARD, addCard),
        takeEvery(PaymentSideEffect.DELETE_CARD, deleteCard),
        takeEvery(PaymentSideEffect.ADD_SUBSCRIPTION, addSubscription),
        takeEvery(PaymentSideEffect.DELETE_SUBSCRIPTION, deleteSubscription),
    ])
}
