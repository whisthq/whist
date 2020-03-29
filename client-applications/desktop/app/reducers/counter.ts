import { Action } from 'redux';
import {
  STORE_USERNAME,
  STORE_IP,
  STORE_IS_USER,
  LOGIN_FAILED,
  STORE_DISTANCE,
  RESET_FEEDBACK,
  SET_OS,
  ASK_FEEDBACK,
  CHANGE_WINDOW,
  STORE_IPINFO,
  STORE_COMPUTERS,
  FETCH_VM_STATUS } from '../actions/counter';

const DEFAULT = {username: '', public_ip: '', warning: false, distance: 0, resetFeedback: false, isUser: true, 
                os: '', askFeedback: false, window: 'main', ipInfo: {}, computers: [], fetchStatus: false}

export default function counter(state = DEFAULT, action: Action<string>) {
  switch (action.type) {
    case STORE_USERNAME:
      return {
      	...state,
      	username: action.username,
      }
    case STORE_IP:
      return {
        ...state,
        public_ip: action.ip
      }
    case STORE_IS_USER:
      return {
        ...state,
        isUser: action.isUser
      }
    case LOGIN_FAILED:
      return {
        ...state,
        warning: action.warning
      }
    case STORE_DISTANCE:
      return {
        ...state,
        distance: action.distance
      }
    case RESET_FEEDBACK:
      return {
        ...state,
        resetFeedback: action.reset
      }
    case SET_OS:
      return {
        ...state,
        os: action.os
      }
    case ASK_FEEDBACK:
      return {
        ...state,
        askFeedback: action.ask
      }
    case CHANGE_WINDOW:
      return {
        ...state,
        window: action.window
      }
    case STORE_IPINFO:
      return {
        ...state,
        ipInfo: action.payload
      }
    case STORE_COMPUTERS:
      return {
        ...state,
        computers: action.payload
      }
    case FETCH_VM_STATUS:
      return {
        ...state,
        fetchStatus: action.status
      }
    default:
      return state;
  }
}
