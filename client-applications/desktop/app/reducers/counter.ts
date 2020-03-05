import { Action } from 'redux';
import {
  STORE_USER_INFO,
  LOGIN_FAILED,
  STORE_DISTANCE,
  RESET_FEEDBACK,
  SET_OS } from '../actions/counter';

const DEFAULT = {username: '', public_ip: '', warning: false, distance: 0, resetFeedback: false, isUser: true, os: ''}

export default function counter(state = DEFAULT, action: Action<string>) {
  switch (action.type) {
    case STORE_USER_INFO:
      console.log("store user info reducer")
      return {
      	...state,
      	username: action.username,
      	public_ip: action.public_ip,
        warning: false,
        isUser: action.is_user
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
    case RESET_FEEDBACK:
      console.log("RESET FEEDBACK REDUCER")
      return {
        ...state,
        resetFeedback: action.reset
      }
    case SET_OS:
      return {
        ...state,
        os: action.os
      }
    default:
      return state;
  }
}
