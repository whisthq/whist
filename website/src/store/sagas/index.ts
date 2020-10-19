import { all } from "redux-saga/effects"

import WaitlistSaga from "store/sagas/waitlist"

export default function* rootSaga() {
    yield all([WaitlistSaga()])
}
