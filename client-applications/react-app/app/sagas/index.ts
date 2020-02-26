import { put, takeLatest, takeEvery, all, call, select, delay } from 'redux-saga/effects';
import { apiPost, apiGet } from '../utils/Api.js'
import * as Action from '../actions/counter';

function* sagaFunction(action) {
  console.log("Saga function called")
}

export default function* rootSaga() {
 	yield all([
     takeEvery(Action.STORE_USER_INFO, sagaFunction),
	]);
}
