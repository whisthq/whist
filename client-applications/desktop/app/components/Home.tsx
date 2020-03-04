import React, {Component} from 'react';
import { connect, bindActionCreators } from 'react-redux';
import { Link, useHistory } from 'react-router-dom';
import { configureStore, history } from '../store/configureStore';
import routes from '../constants/routes.json';
import styles from './Home.css';
import Titlebar from 'react-electron-titlebar';
import Background from '../../resources/images/background.jpg';
import Logo from "../../resources/images/logo.svg";
import UserIcon from "../../resources/images/user.svg";
import LockIcon from "../../resources/images/lock.svg";

import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSpinner, faWindowMinimize, faTimes } from '@fortawesome/free-solid-svg-icons'

import { storeUserInfo, loginUser } from "../actions/counter"

class Home extends Component {
  constructor(props) {
    super(props)
    this.state = {username: '', password: '', loggingIn: false, warning: false, version: "1.0.0"}
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
    console.log("login trigger")
  	this.setState({loggingIn: true, warning: false});
  	this.props.dispatch(loginUser(this.state.username, this.state.password))
  }

  LoginKeyPress = (event) => {
  	if(event.key === 'Enter') {
  		this.LoginUser()
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

  componentDidMount() {
    var appVersion = require('../package.json').version;
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
	        <div style = {{textAlign: 'right', paddingTop: 10, paddingRight: 20}}>
	          <div onClick = {this.MinimizeWindow} style = {{display: 'inline', paddingRight: 25, position: 'relative', bottom: 6}}>
	             <FontAwesomeIcon className = {styles.windowControl} icon={faWindowMinimize} style = {{color: '#999999', height: 10}}/>
	          </div>
	          <div onClick = {this.CloseWindow} style = {{display: 'inline'}}>
	             <FontAwesomeIcon className = {styles.windowControl} icon={faTimes} style = {{color: '#999999', height: 16}}/>
	          </div>
	        </div>
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
		        <button onClick = {() => this.LoginUser()} type = "button" className = {styles.loginButton} id = "login-button">START</button>
		      </div>
          <div className = {styles.rememberMeContainer}>
            <input type = "checkbox" className = {styles.rememberMe}/> Remember me
          </div>
		      <div style = {{fontSize: 12, color: "#D6D6D6", width: 250, margin: 'auto'}}>
		      {
		      	this.state.loggingIn && !this.props.warning
		      	?
		      	<div>
		      		<FontAwesomeIcon icon={faSpinner} spin style = {{color: "#5EC4EB", marginRight: 4, width: 12}}/> Logging In
		      	</div>
		      	:
		      	<div>
		      	</div>
		      }
		      {
		      	this.props.warning
		      	?
		      	<div>
		      		Invalid credentials. If you lost your password, you can reset it on the Fractal website.
		      	</div>
		      	:
		      	<div>
		      	</div>
		      }
		      </div>
		    </div>
		</div>
	);
	}
}

function mapStateToProps(state) {
  return { 
    username: state.counter.username,
    public_ip: state.counter.public_ip,
    warning: state.counter.warning
  }
}

export default connect(mapStateToProps)(Home);
