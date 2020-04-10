import { Action } from 'redux';
import * as MainAction from '../actions/counter';

const DEFAULT = {username: '', public_ip: '', warning: false, distance: 0, resetFeedback: false, isUser: true, 
                os: '', askFeedback: false, window: 'main', ipInfo: {}, computers: [], fetchStatus: false, disk: '',
                attachState: 'NOT_REQUESTED'}

export default function counter(state = DEFAULT, action: Action<string>) {
  switch (action.type) {
    case MainAction.STORE_USERNAME:
      return {
      	...state,
      	username: action.username,
      }
    case MainAction.STORE_IP:
      return {
        ...state,
        public_ip: action.ip
      }
    case MainAction.STORE_IS_USER:
      return {
        ...state,
        isUser: action.isUser
      }
    case MainAction.LOGIN_FAILED:
      return {
        ...state,
        warning: action.warning
      }
    case MainAction.STORE_DISTANCE:
      return {
        ...state,
        distance: action.distance
      }
    case MainAction.RESET_FEEDBACK:
      return {
        ...state,
        resetFeedback: action.reset
      }
    case MainAction.SET_OS:
      return {
        ...state,
        os: action.os
      }
    case MainAction.ASK_FEEDBACK:
      return {
        ...state,
        askFeedback: action.ask
      }
    case MainAction.CHANGE_WINDOW:
      return {
        ...state,
        window: action.window
      }
    case MainAction.STORE_IPINFO:
      return {
        ...state,
        ipInfo: action.payload
      }
    case MainAction.STORE_COMPUTERS:
      return {
        ...state,
        computers: action.payload
      }
    case MainAction.FETCH_DISK_STATUS:
      return {
        ...state,
        fetchStatus: action.status
      }
    case MainAction.STORE_DISK_NAME:
      return {
        ...state,
        disk: action.disk
      }
    case MainAction.ATTACH_DISK:
      return {
        ...state,
        attachState: action.state
      }
    default:
      return state;
  }
}
