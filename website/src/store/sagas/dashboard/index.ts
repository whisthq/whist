import { all } from "redux-saga/effects"

import PaymentSaga from "store/sagas/dashboard/payment"

// in the future there will be options for
// cloud storage, account settings, support tickets, etcetera
// these will be done from the dashboard and combined here
export default function* rootSaga() {
    yield all([PaymentSaga()])
}
