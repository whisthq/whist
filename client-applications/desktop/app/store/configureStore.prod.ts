import { createStore, applyMiddleware } from "redux";
import createSagaMiddleware from "redux-saga";
import { createHashHistory } from "history";
import { routerMiddleware } from "connected-react-router";
import createRootReducer from "reducers/index";
import { Store, counterStateType } from "reducers/types";
import rootSaga from "sagas/index";

const history = createHashHistory();
const rootReducer = createRootReducer(history);
const router = routerMiddleware(history);
const sagaMiddleware = createSagaMiddleware();
const enhancer = applyMiddleware(sagaMiddleware);

function configureStore(initialState?: counterStateType): Store {
  const store = createStore(rootReducer, initialState, enhancer);
  sagaMiddleware.run(rootSaga);
  return store;
}

export default { configureStore, history };
