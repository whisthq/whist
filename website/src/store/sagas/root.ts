import { all } from "redux-saga/effects"

import WaitlistSaga from "store/sagas/waitlist"
import AuthSaga from "store/sagas/auth"

export default function* rootSaga() {
    yield all([AuthSaga(), WaitlistSaga()])
}
