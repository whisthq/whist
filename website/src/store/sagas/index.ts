import { all } from "redux-saga/effects";

import JoinSaga from "store/sagas/waitlist/join_saga";

export default function* rootSaga() {
    yield all([JoinSaga()]);
}
