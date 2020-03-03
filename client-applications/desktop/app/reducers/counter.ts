import { Action } from 'redux';
import { 
  STORE_USER_INFO, 
  LOGIN_FAILED, 
  STORE_DISTANCE } from '../actions/counter';

const DEFAULT = {username: '', public_ip: '', warning: false, distance: 0}

export default function counter(state = DEFAULT, action: Action<string>) {
  switch (action.type) {
    case STORE_USER_INFO:
      console.log("store user info reducer")
      return {
      	...state,
      	username: action.username,
      	public_ip: action.public_ip,
        warning: false
      }
    case LOGIN_FAILED:
      console.log("login fail reducer")
      return {
        ...state,
        warning: true
      }
    case STORE_DISTANCE:
      console.log("store distance reducer")
      return {
        ...state,
        distance: action.distance
      }
    default:
      return state;
  }
}
