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
import { faSpinner} from '@fortawesome/free-solid-svg-icons'

import { storeUserInfo } from "../actions/counter"

class Home extends Component {
  constructor(props) {
    super(props)
    this.state = {username: '', password: '', loggingIn: false, warning: false}
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
  	let component = this;
    const url = 'https://cube-celery-vm.herokuapp.com/user/login';
    const body = {
        username: this.state.username,
        password: this.state.password
    }

    var xhr = new XMLHttpRequest();

    xhr.onreadystatechange = function () {
        if (this.readyState != 4) return;

        if (this.status == 200) {
        	history.push("/counter");
            var data = JSON.parse(this.responseText)
            component.props.dispatch(storeUserInfo(data.username, data.public_ip))
            component.setState({loggingIn: false})
        } else {
        	console.log("Invalid credentials")
            component.setState({loggingIn: false, warning: true})
        }
    };

    xhr.open("POST", url, true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(body));
  }

  LoginKeyPress = (event) => {
  	console.log("Key pressed")
  	if(event.key === 'Enter') {
  		console.log("Enter key pressed")
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

  componentDidMount() {
  	if(this.props.username && this.props.public_ip) {
  		history.push('/counter')
  	}
  }

  render() {
	return (
		<div className={styles.container} data-tid="container" style = {{backgroundImage: "url(" + Background + ")"}}>
			<div>
			  <Titlebar backgroundColor="#000000"/>
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
		      <div style = {{fontSize: 12, color: "#D6D6D6", width: 250, margin: 'auto'}}>
		      {
		      	this.state.loggingIn
		      	?
		      	<div>
		      		<FontAwesomeIcon icon={faSpinner} spin style = {{color: "#5EC4EB", marginRight: 4}}/> Logging In
		      	</div>
		      	:
		      	<div>
		      	</div>
		      }
		      {
		      	this.state.warning
		      	?
		      	<div>
		      		Invalid credentials. If you lost your password, you can reset it on the website.
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
  console.log(state)
  return { 
    username: state.counter.username,
    public_ip: state.counter.public_ip
  }
}

export default connect(mapStateToProps)(Home);
