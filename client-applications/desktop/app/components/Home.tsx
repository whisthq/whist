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
import UpdateScreen from './custom_components/UpdateScreen.tsx'

import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch, faWindowMinimize, faTimes } from '@fortawesome/free-solid-svg-icons'

import { loginUser, setOS, loginStudio, loginFailed } from "../actions/counter"

class Home extends Component {
  constructor(props) {
    super(props)
    this.state = {username: '', password: '', loggingIn: false, warning: false, version: "1.0.0", 
                  studios: false, rememberMe: false, live: false}
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
    const storage = require('electron-json-storage');
    this.props.dispatch(loginFailed(false));
  	this.setState({loggingIn: true});
    if(this.state.rememberMe) {
      storage.set('credentials', {username: this.state.username, password: this.state.password})
    } else {
      storage.set('credentials', {username: '', password: ''})
    }
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

  changeRememberMe = (event) => {
    const target = event.target;
    if(target.checked) {
      console.log("Remember me!")
      this.setState({rememberMe: true})
    } else {
      this.setState({rememberMe: false})
    }
  }

  componentDidMount() {
    const ipc = require('electron').ipcRenderer;
    const storage = require('electron-json-storage');

    let component = this;

    var appVersion = require('../package.json').version;
    const os = require('os')
    this.props.dispatch(setOS(os.platform()))
    this.setState({version: appVersion})

    storage.get('credentials', function(error, data) {
      if (error) throw error;
      console.log(data)

      if(data && Object.keys(data).length > 0) {
        if(data.username != '' && data.password != '' && component.state.live) {
          component.setState({username: data.username, password: data.password, loggingIn: true, warning: false}, function() {
            component.props.dispatch(loginUser(component.state.username, component.state.password))
          })
        }
      }
    });

  	if(this.props.username && this.props.public_ip && component.state.live)  {
  		history.push('/counter')
  	}
  }

  render() {
	return (
		<div className={styles.container} data-tid="container" style = {{backgroundImage: "url(" + Background + ")"}}>
      <UpdateScreen/>
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
        <div className={styles.macTitleBar}/>
      }
      {
      this.state.live
      ?
      <div className={styles.removeDrag}>
		    <div className = {styles.landingHeader}>
		      <div className = {styles.landingHeaderLeft}>
		        <img src = {Logo} width = "20" height = "20"/>
		        <span className = {styles.logoTitle}>Fractal</span>
		      </div>
		      <div className = {styles.landingHeaderRight}>
		        <span id = "forgotButton" onClick = {this.ForgotPassword}>Forgot Password?</span>
		        <button type = "button" className = {styles.signupButton}  style = {{borderRadius: 5, marginLeft: 15}} id = "signup-button" onClick = {this.SignUp}>Sign Up</button>
		      </div>
		    </div>
        <div style = {{marginTop: 50}}>
          {
          this.state.studios
          ?
          <div style = {{margin: 'auto', display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(false)} style = {{color: '#DADADA', marginRight: 20, paddingBottom: 8, width: 90}}>Personal</div>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(true)} style = {{color: 'white', fontWeight: 'bold', marginLeft: 20, borderBottom: 'solid 3px #5EC4EB', paddingBottom: 8, width: 90}}>Teams</div>
          </div>
          :
          <div style = {{margin: 'auto', display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(false)} style = {{color: 'white', fontWeight: 'bold', marginRight: 20, borderBottom: 'solid 3px #5EC4EB', paddingBottom: 8, width: 90}}>Personal</div>
              <div className = {styles.tabHeader} onClick = {() => this.ToggleStudio(true)} style = {{color: '#DADADA', marginLeft: 20, paddingBottom: 8, width: 90}}>Teams</div>
          </div>
          }
  		    <div className = {styles.loginContainer}>
  		      <div>
  		        <img src = {UserIcon} width = "100" className = {styles.inputIcon}/>
  		        <input onKeyPress = {this.LoginKeyPress} onChange = {this.UpdateUsername} type = "text" className = {styles.inputBox} style = {{borderRadius: 5}} placeholder = "Username" id = "username"/>
  		      </div>
  			  <div>
  		        <img src = {LockIcon} width = "100" className = {styles.inputIcon}/>
  		        <input onKeyPress = {this.LoginKeyPress} onChange = {this.UpdatePassword} type = "password" className = {styles.inputBox} style = {{borderRadius: 5}} placeholder = "Password" id = "password"/>
  		      </div>
  		      <div style = {{marginBottom: 20}}>
              {
              !this.state.studios 
              ?
              (
              this.state.loggingIn && !this.props.warning
              ?
              <button type = "button" className = {styles.loginButton} id = "login-button" style = {{opacity: 0.6, textAlign: 'center'}}>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "white", width: 12, marginRight: 5, position: 'relative', top: 0.5}}/> Processing 
              </button>
              :
  		        <button onClick = {() => this.LoginUser()} type = "button" className = {styles.loginButton} id = "login-button">START</button>
              )
              :
              <button type = "button" className = {styles.loginButton} id = "login-button" style = {{opacity: 0.5, background: 'linear-gradient(258.54deg, #5ec3eb 0%, #5ec3eb 100%)', borderRadius: 5}}>
                START
              </button>
              }
  		      </div>
            <div style = {{marginTop: 25, display: 'flex', justifyContent: 'center', alignItems: 'center'}}>
              <label className = {styles.termsContainer}>
                <input
                  type="checkbox"
                  onChange={this.changeRememberMe}
                  onKeyPress = {this.LoginKeyPress}
                  /> 
                <span className = {styles.checkmark}></span>
              </label>

              <div style = {{fontSize: 12}}>
                Remember Me
              </div>
            </div>
  		      <div style = {{fontSize: 12, color: "#D6D6D6", width: 250, margin: 'auto', marginTop: 15}}>
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
		</div>
    :
    <div style = {{lineHeight: 1.5, margin: '150px auto'}}> We are currently pushing out a critical Linux update. Your app will be back online very soon. We apologize for the inconvenience!
    </div>
    }
	);
	}
}

function mapStateToProps(state) {
  console.log(this.state)
  return {
    username: state.counter.username,
    public_ip: state.counter.public_ip,
    warning: state.counter.warning,
    os: state.counter.os
  }
}

export default connect(mapStateToProps)(Home);
