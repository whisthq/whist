import { put, takeEvery, all, call, select } from "redux-saga/effects"
import { apiPost } from "shared/utils/api"
import history from "shared/utils/history"

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
                    cards: state.DashboardReducer.stripeInfo.cards + 1,
                })
            )
        } else {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    stripeRequestRecieved: true,
                    stripeStatus: "failure",
                    cards: state.DashboardReducer.stripeInfo.cards + 1,
                })
            )
        }
    }
}

function* deleteCard(action: any) {
    const state = yield select()

    const { json } = yield call(
        apiPost,
        "/stripe/deleteCard",
        {
            email: state.AuthReducer.user.user_id,
            token: action.token,
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
                cards: state.DashboardReducer.stripeInfo.cards - 1,
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
            token: action.token,
            email: state.AuthReducer.user.user_id,
            plan: action.plan,
            code: action.code,
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
            history.push("/dashboard")
            yield put(
                PaymentPureAction.updateStripeInfo({
                    subscription: action.plan,
                })
            )
        }
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
        yield put(
            PaymentPureAction.updateStripeInfo({
                stripeRequestRecieved: true,
            })
        )

        if (json.status === 200) {
            yield put(
                PaymentPureAction.updateStripeInfo({
                    subscription: null,
                })
            )
        }
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
