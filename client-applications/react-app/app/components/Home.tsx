import React, {Component} from 'react';
import { connect } from 'react-redux';
import { Link, useHistory } from 'react-router-dom';
import { configureStore, history } from '../store/configureStore';
import routes from '../constants/routes.json';
import styles from './Home.css';
import Titlebar from 'react-electron-titlebar';
import Background from '../../resources/images/background.jpg';
import Logo from "../../resources/images/logo.svg";
import UserIcon from "../../resources/images/user.svg";
import LockIcon from "../../resources/images/lock.svg";

class Home extends Component {
  constructor(props) {
    super(props)
    this.state = {username: '', password: '', loggingIn: false}
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
            console.log(data)
        } else {
            console.log("Invalid credentials")
        }
    };

    xhr.open("POST", url, true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(body));
  }

  HandleLogin() {

  }

  LoginKeyPress = (evt) => {
  	if(evt.key === 'Enter') {
  		this.LoginUser()
  	}
  }

  componentDidMount() {
  	if(this.props.username && this.props.public_ip) {
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
		        <span id = "forgotButton">Forgot Password?</span> 
		        <button type = "button" className = {styles.signupButton} id = "signup-button">Sign Up</button>
		      </div>
		    </div>
		    <div className = {styles.loginContainer}>
		      <div>
		        <img src = {UserIcon} width = "100" className = {styles.inputIcon}/>
		        <input onChange = {this.UpdateUsername} type = "text" className = {styles.inputBox} placeholder = "Username" id = "username"/>
		      </div>
			  <div>
		        <img src = {LockIcon} width = "100" className = {styles.inputIcon}/>
		        <input onChange = {this.UpdatePassword} type = "password" className = {styles.inputBox} placeholder = "Password" id = "password"/>
		      </div>
		      <div>
		        <button onClick = {() => this.LoginUser()} onKeyPress = {this.LoginKeyPress} type = "button" className = {styles.loginButton} id = "login-button">START</button>
		      </div>
		      {
		      	this.state.loggingIn
		      	?
		      	<div>
		      	</div>
		      	:
		      	<div style = {{marginTop: 15, fontSize: 12, color: "#D6D6D6"}}>
		      		Logging In
		      	</div>
		      }
		    </div>
		</div>
	);
	}
}


export default Home;
