import React, {Component} from 'react';
import { connect, bindActionCreators } from 'react-redux';
import { Link, useHistory } from 'react-router-dom';
import { configureStore, history } from '../store/configureStore';
import InputGroup from 'react-bootstrap/InputGroup'
import FormControl from 'react-bootstrap/FormControl'
import { Tab, Tabs, TabList, TabPanel } from 'react-tabs';

import routes from '../constants/routes.json';
import styles from './Home.css';
import Titlebar from 'react-electron-titlebar';
import Background from '../../resources/images/background.jpg';
import Logo from "../../resources/images/logo.svg";
import UserIcon from "../../resources/images/user.svg";
import LockIcon from "../../resources/images/lock.svg";


import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch, faWindowMinimize, faTimes } from '@fortawesome/free-solid-svg-icons'

import { loginUser, setOS, loginStudio } from "../actions/counter"

class Home extends Component {
  constructor(props) {
    super(props)
    this.state = {username: '', password: '', loggingIn: false, warning: false, version: "1.0.0", updateScreen: false,
                  percentageLeft: 300, percentageDownloaded: 0, downloadSpeed: 0, transferred: 0, total: 0, 
                  downloadError: '', studios: false}
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


  UpdateUsername = (evt) => {
  	this.setState({
  		username: evt.target.value
  	})
  }

  UpdatePassword = (evt) => {
  	this.setState({
  		password: evt.target.value
  	})
  }

  LoginUser = () => {
  	this.setState({loggingIn: true, warning: false});
  	this.props.dispatch(loginUser(this.state.username, this.state.password))
  }

  LoginKeyPress = (event) => {
  	if(event.key === 'Enter' && !this.state.studios) {
  		this.LoginUser()
  	}
    if(event.key === 'Enter' && this.state.studios) {
      // this.LoginStudio()
    }
  }

  ForgotPassword = () => {
  	const {shell} = require('electron')
  	shell.openExternal('https://www.fractalcomputers.com/reset')
  }

  SignUp = () => {
  	const {shell} = require('electron')
  	shell.openExternal('https://www.fractalcomputers.com/auth')
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

  ToggleStudio = (isStudio) => {
    this.setState({studios: isStudio})
  }

  LoginStudio = () => {
    this.setState({loggingIn: true, warning: false});
    this.props.dispatch(loginStudio(this.state.username, this.state.password))
  }

  componentDidMount() {
    const ipc = require('electron').ipcRenderer;
    let component = this;

    ipc.on('update', (event, update) => {
      if(update) {
        component.setState({updateScreen: true})
      }
    })

    ipc.on('percent', (event, percent) => {
      percent = percent * 3;
      this.setState({percentageLeft: 300 - percent, percentageDownloaded: percent})
    })

    ipc.on('download-speed', (event, speed) => {
      this.setState({downloadSpeed: Math.round(speed / 1000000)})
    })

    ipc.on('transferred', (event, transferred) => {
      this.setState({transferred: Math.round(transferred / 1000000)})
    })

    ipc.on('total', (event, total) => {
      this.setState({total: Math.round(total / 1000000)})
    })

    ipc.on('error', (event, error) => {
      this.setState({downloadError: error})
    })

    ipc.on('downloaded', (event, downloaded) => {
      console.log("UPDATE DOWNLOADED")
    })

    var appVersion = require('../package.json').version;
    const os = require('os')
    this.props.dispatch(setOS(os.platform()))
    this.setState({version: appVersion})
  	if(this.props.username && this.props.public_ip) {
  		history.push('/counter')
  	}
  }

  render() {
	return (
		<div className={styles.container} data-tid="container" style = {{backgroundImage: "url(" + Background + ")"}}>
      <div style = {{position: 'absolute', bottom: 15, right: 15, fontSize: 11, color: "#D1D1D1"}}>
        Version: {this.state.version}
      </div>
      {
        this.props.os === 'win32'
        ?
        <div>
          <Titlebar backgroundColor="#000000"/>
        </div>
        :
        <div style = {{marginTop: 10}}></div>
      }
		    <div className = {styles.landingHeader}>
		      <div className = {styles.landingHeaderLeft}>
		        <img src = {Logo} width = "20" height = "20"/>
		        <span className = {styles.logoTitle}>Fractal</span>
		      </div>
		      <div className = {styles.landingHeaderRight}>
		        <span id = "forgotButton" onClick = {this.ForgotPassword}>Forgot Password?</span>
		        <button type = "button" className = {styles.signupButton} id = "signup-button" onClick = {this.SignUp}>Sign Up</button>
		      </div>
		    </div>
        {
        !this.state.updateScreen
        ?
        <div style = {{marginTop: 50}}>
          {
          this.state.studios
          ?
          <div style = {{margin: 'auto', display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(false)} style = {{color: '#DADADA', marginRight: 20, paddingBottom: 8, width: 90}}>Personal</div>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(true)} style = {{color: 'white', fontWeight: 'bold', marginLeft: 20, borderBottom: 'solid 3px #5EC4EB', paddingBottom: 8, width: 90}}>For Studios</div>
          </div>
          :
          <div style = {{margin: 'auto', display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(false)} style = {{color: 'white', fontWeight: 'bold', marginRight: 20, borderBottom: 'solid 3px #5EC4EB', paddingBottom: 8, width: 90}}>Personal</div>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(true)} style = {{color: '#DADADA', marginLeft: 20, paddingBottom: 8, width: 90}}>For Studios</div>
          </div>
          }
  		    <div className = {styles.loginContainer}>
  		      <div>
  		        <img src = {UserIcon} width = "100" className = {styles.inputIcon}/>
  		        <input onKeyPress = {this.LoginKeyPress} onChange = {this.UpdateUsername} type = "text" className = {styles.inputBox} placeholder = "Username" id = "username"/>
  		      </div>
  			  <div>
  		        <img src = {LockIcon} width = "100" className = {styles.inputIcon}/>
  		        <input onKeyPress = {this.LoginKeyPress} onChange = {this.UpdatePassword} type = "password" className = {styles.inputBox} placeholder = "Password" id = "password"/>
  		      </div>
  		      <div style = {{marginBottom: 20}}>
              {
              !this.state.studios 
              ?
  		        <button onClick = {() => this.LoginUser()} type = "button" className = {styles.loginButton} id = "login-button">START</button>
              :
              <button type = "button" className = {styles.loginButton} id = "login-button" style = {{opacity: 0.5, background: 'linear-gradient(258.54deg, #5ec3eb 0%, #5ec3eb 100%)'}}>
                START
              </button>
              }
  		      </div>
  		      <div style = {{fontSize: 12, color: "#D6D6D6", width: 250, margin: 'auto'}}>
  		      {
  		      	this.state.loggingIn && !this.props.warning
  		      	?
  		      	<div>
  		      		<FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "#5EC4EB", marginRight: 4, width: 12}}/> Logging In
  		      	</div>
  		      	:
  		      	<div>
  		      	</div>
  		      }
  		      {
  		      	this.props.warning
  		      	?
              (
              this.state.studios 
              ?
  		      	<div>
  		      		Invalid credentials. If you lost your password, you can reset it on the&nbsp;
                <div onClick = {this.ForgotPassword} className = {styles.pointerOnHover} style = {{display: 'inline', fontWeight: 'bold'}}>Fractal website.</div>
  		      	</div>
              :
              <div>
                Invalid credentials. If you lost your password, you can reset it on the&nbsp;
                <div onClick = {this.ForgotPassword} className = {styles.pointerOnHover} style = {{display: 'inline', fontWeight: 'bold'}}>Fractal website.</div>
              </div>
              )
  		      	:
  		      	<div>
  		      	</div>
  		      }
  		      </div>
  		    </div>
        </div>
        :
        <div  style = {{position: 'relative'}}>
          <div style = {{paddingTop: 180, textAlign: 'center', color: 'white', width: 900}}>
            <div style = {{display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div style = {{width: `${ this.state.percentageDownloaded }px`, height: 6, background: 'linear-gradient(258.54deg, #5ec3eb 0%, #d023eb 100%)'}}>
              </div>
              <div style = {{width: `${ this.state.percentageLeft }px`, height: 6, background: '#111111'}}>
              </div>
            </div>
            {
            this.state.downloadError === ''
            ?
            <div style = {{marginTop: 10, fontSize: 14, display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div style = {{color: "#D6D6D6"}}>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "#5EC4EB", marginRight: 4, width: 12}}/>  Downloading Update ({this.state.downloadSpeed} Mbps)
              </div>
            </div>
            :
            <div style = {{marginTop: 10, fontSize: 14, display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div style = {{color: "#D6D6D6"}}>
                {this.state.downloadError}
              </div>
            </div>
            }  
            <div style = {{color: "#C9C9C9", fontSize: 10, margin: 'auto', marginTop: 5}}>
              {this.state.transferred} / {this.state.total} Bytes Downloaded
            </div>
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
    warning: state.counter.warning,
    os: state.counter.os
  }
}

export default connect(mapStateToProps)(Home);
