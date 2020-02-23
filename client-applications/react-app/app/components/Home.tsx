import React from 'react';
import { Link } from 'react-router-dom';
import routes from '../constants/routes.json';
import styles from './Home.css';
import Titlebar from 'react-electron-titlebar';
import Background from '../../resources/images/background.jpg';
import Logo from "../../resources/images/logo.svg";
import UserIcon from "../../resources/images/user.svg";
import LockIcon from "../../resources/images/lock.svg";

export default function Home() {
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
	        <input type = "text" className = {styles.inputBox} placeholder = "Username" id = "username"/>
	      </div>
		  <div>
	        <img src = {LockIcon} width = "100" className = {styles.inputIcon}/>
	        <input type = "password" className = {styles.inputBox} placeholder = "Password" id = "password"/>
	      </div>
	      <div>
	        <Link to={routes.COUNTER}><button type = "button" className = {styles.loginButton} id = "login-button">START</button></Link>
	      </div>
	      <div className = {styles.rememberMeContainer}>
	        <input type = "checkbox" id = "remember-me" className = {styles.rememberMe}/> Remember me
	      </div>
	    </div>
	</div>
  );
}
