import React, {Component} from 'react';
import { Link } from 'react-router-dom';
import { connect } from 'react-redux';
import styles from './Counter.css';
import routes from '../constants/routes.json';
import Row from 'react-bootstrap/Row'
import Col from 'react-bootstrap/Col'
import { configureStore, history } from '../store/configureStore';

import Titlebar from 'react-electron-titlebar';
import Logo from "../../resources/images/logo.svg";
import Spinner from "../../resources/images/spinner.svg";
import WifiBox from "./custom_components/WifiBox.tsx";
import DistanceBox from "./custom_components/DistanceBox.tsx";
import CPUBox from "./custom_components/CPUBox.tsx";
import Typeform from "./custom_components/Typeform.tsx"
import MainBox from "./custom_components/MainBox.tsx"
import UpdateScreen from './custom_components/UpdateScreen.tsx'

import { Offline, Online } from "react-detect-offline";
import Popup from "reactjs-popup"
import { ReactTypeformEmbed } from 'react-typeform-embed'

import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSpinner, faCheck, faArrowRight } from '@fortawesome/free-solid-svg-icons'

import { storeUsername, storeIP, storeIsUser, trackUserActivity, storeDistance, askFeedback, changeWindow, fetchDiskStatus, storeDiskName } from "../actions/counter"

class Counter extends Component {
  constructor(props) {
    super(props)
    this.state = {isLoading: true, username: '', launches: 0}
  }

  CloseWindow = () => {
    const remote = require('electron').remote
    let win = remote.getCurrentWindow()

    win.close()
  }

  MinimizeWindow = () => {
    const remote = require('electron').remote
    let win = remote.getCurrentWindow()

    win.minimize()
  }

  OpenFeedback = () => {
    this.props.dispatch(askFeedback(true))
  }

  OpenSettings = () => {
    this.props.dispatch(changeWindow('settings'))
  }

  LogOut = () => {
    this.props.dispatch(storeUsername(null))
    this.props.dispatch(storeIP(''))
    this.props.dispatch(storeIsUser(true))
    this.props.dispatch(fetchDiskStatus(false))
    this.props.dispatch(storeDiskName(''))
    const storage = require('electron-json-storage');
    storage.set('credentials', {username: '', password: ''}, function(err) {
      console.log("trying to go back")
      console.log(err)
      history.push("/");
    })
  }

  componentDidMount() {
    this.props.dispatch(changeWindow('main'))
    this.setState({isLoading: false})
    if(this.props.username && this.props.username != '') {
      this.setState({username: this.props.username.split('@')[0]})
    }
  }

  componentDidUpdate(prevProps) {
    if(prevProps.username == '' && this.props.username != '' && this.state.username == '') {
      this.setState({username: this.props.username.split('@')[0]})
    }
  }

  render() {
    const barHeight = 5

    return (
      <div className={styles.container} data-tid="container" style = {{fontFamily: "Maven Pro"}}>
        <UpdateScreen/>
        {
        this.props.os === 'win32'
        ?
        <div>
          <Titlebar backgroundColor="#000000"/>
        </div>
        :
        <div style = {{marginTop: 10}}></div>
        }
        {
        this.state.isLoading
        ?
        <div>
        </div>
        :
        <div>
        <Typeform/>
        <div className = {styles.landingHeader}>
          <div className = {styles.landingHeaderLeft}>
            <img src = {Logo} width = "20" height = "20"/>
            <span className = {styles.logoTitle}>Fractal</span>
          </div>
          <div className = {styles.landingHeaderRight}>
            <span className = {styles.headerButton} onClick = {this.OpenFeedback}>Support</span>
            <span className = {styles.headerButton} onClick = {this.OpenSettings}>Settings</span>
            <Popup trigger = {
            <span className = {styles.headerButton}>
              Refer a Friend
             </span>
            } modal contentStyle = {{width: 300, borderRadius: 5, backgroundColor: "#111111", border: "none", height: 100, padding: 30, textAlign: "center"}}>
              <div style = {{fontWeight: 'bold', fontSize: 20}} className = {styles.blueGradient}><strong>Coming Soon</strong></div>
              <div style = {{fontSize: 12, color: "#D6D6D6", marginTop: 20}}>Get rewarded when you refer a friend.</div>
            </Popup>
            <button onClick = {this.LogOut} type = "button" className = {styles.signupButton} id = "signup-button" style = {{marginLeft: 25, fontFamily: "Maven Pro", fontWeight: 'bold'}}>Sign Out</button>
          </div>
        </div>
        <div style = {{display: 'flex', padding: '20px 75px' }}>
          <div style = {{width: '65%', textAlign: 'left', paddingRight: 20}}>
            <MainBox currentWindow = {this.props.currentWindow} default = "main"/>
          </div>
          {
          this.props.currentWindow === 'main'
          ?
          <div className = {styles.statBox} style = {{width: '35%', textAlign: 'left', background: "linear-gradient(217.69deg, #363868 0%, rgba(30, 31, 66, 0.5) 101.4%)", borderRadius: 5, padding: 30, minHeight: 350}}>
            <div style = {{fontWeight: 'bold', fontSize: 18}}>
              Welcome, {this.state.username}
            </div>
            <div style = {{marginTop: 10, display: "inline-block"}}>
              <Online>
                <div style = {{background: "none", border: "solid 1px #3ce655", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3.5}}>
                </div>
                <div style = {{display: "inline", float: "left", marginLeft: 5, fontSize: 12, color: "#D6D6D6"}}>
                  Online
                </div>
              </Online>
              <Offline>
                <div style = {{background: "none", border: "solid 1px #3ce655", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3.5}}>
                </div>
                <div style = {{display: "inline", float: "left", marginLeft: 5, fontSize: 12, color: "#D6D6D6"}}>
                  Offline
                </div>
              </Offline>
            </div>

            <WifiBox barHeight = {barHeight}/>
            <DistanceBox barHeight = {barHeight} public_ip = {this.props.public_ip}/>
            <CPUBox barHeight = {barHeight}/>

          </div>
          :
          <div></div>
          }
        </div>
        </div>
        }
      </div>
    );
  }
}

function mapStateToProps(state) {
  return {
    username: state.counter.username,
    public_ip: state.counter.public_ip,
    os: state.counter.os,
    askFeedback: state.counter.askFeedback,
    currentWindow: state.counter.window,
    disk: state.counter.disk
  }
}

export default connect(mapStateToProps)(Counter);
