import React, {Component} from 'react';
import { Link } from 'react-router-dom';
import { connect } from 'react-redux';
import styles from './Counter.css';
import routes from '../constants/routes.json';
import Row from 'react-bootstrap/Row'
import Col from 'react-bootstrap/Col'
import { configureStore, history } from '../store/configureStore';
import * as geolib from 'geolib';

import Titlebar from 'react-electron-titlebar';
import Logo from "../../resources/images/logo.svg";
import Car from "../../resources/images/car.jpg";
import Folder from "../../resources/images/folder.svg";
import Video from "../../resources/images/video.svg";
import Spinner from "../../resources/images/spinner.svg";

import { Offline, Online } from "react-detect-offline";
import Popup from "reactjs-popup"
import { ReactTypeformEmbed } from 'react-typeform-embed'

import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faSpinner, faWindowMinimize, faTimes, faCheck, faArrowRight } from '@fortawesome/free-solid-svg-icons'

import { storeUserInfo, trackUserActivity, storeDistance, resetFeedback, sendFeedback } from "../actions/counter"

class Counter extends Component {
  constructor(props) {
    super(props)
    this.state = {isLoading: true, username: '', internetspeed: 0, distance: 0, internetbar: 50, distancebar: 50, cores: 0, corebar: 40, askFeedback: false, feedback: '', feedbackThankYou: false, launches: 0}
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

  MeasureConnectionSpeed = () => {
    let component = this;
    var imageAddr = "http://www.kenrockwell.com/contax/images/g2/examples/31120037-5mb.jpg";
    var downloadSize = 4995374; //bytes
    var startTime, endTime;
    var download = new Image();
    download.onload = function () {
        endTime = (new Date()).getTime();
        var duration = (endTime - startTime) / 1000;
        var bitsLoaded = downloadSize * 8;
        var speedBps = (bitsLoaded / duration).toFixed(2);
        var speedKbps = (speedBps / 1024).toFixed(2);
        var speedMbps = (speedKbps / 1024).toFixed(2);
        component.setState({internetspeed: speedMbps});
        component.setState({internetbar: Math.min(Math.max(speedMbps, 50), 200)})
    }

    startTime = (new Date()).getTime();
    var cacheBuster = "?nnn=" + startTime;
    download.src = imageAddr + cacheBuster;
  }

  MeasureCores = () => {
    const si = require('systeminformation')
    si.cpu().then(data => {
      this.setState({cores: data.cores})
      this.setState({corebar: Math.min(Math.max(40, 200 * (data.cores / 32)), 200)})
    })
  }

  CalculateDistance = (public_ip) => {
    let component = this;
    const iplocation = require("iplocation").default;
    var options = {
      enableHighAccuracy: true,
      timeout: 10000
    }
    var metersInMile = 1609.34;
    var dist;

    navigator.geolocation.getCurrentPosition(showPosition, showError, options);

    function showPosition(position) {
      iplocation(public_ip).then(function(res) {
        dist = geolib.getDistance(
        {
          latitude: position.coords.latitude,
          longitude: position.coords.longitude
        },
        {
          latitude: res.latitude,
          longitude: res.longitude,
        });
        dist = Math.round(dist / metersInMile)
        component.setState({distance: dist,
          distancebar: Math.min(Math.max(220 * dist / 1000, 80), 220)})
      })
    }

    function showError(error) {
    }
  }


  LaunchProtocol = () => {
    if(this.state.launches == 0) {
      var child      = require('child_process').spawn;
      var appRootDir = require('electron').remote.app.getAppPath();
      const os       = require('os');

      // check which OS we're on to properly launch the protocol
      if (os.platform() === 'darwin') { // mac
        var path =  appRootDir + "/fractal-protocol/desktop/desktop"
        path = path.replace('/Resources/app.asar','');
        path = path.replace('/desktop/app', '/desktop')
      }
      else if (os.platform() === 'win32') { // windows
        var path = process.cwd() + "\\fractal-protocol\\desktop\\desktop.exe"
      }
      else { // linux
        var path = "TODO"
      }

      var parameters = [this.props.public_ip, 123]

      if(this.props.isUser) {
        this.TrackActivity(true);
      }
      const protocol = child(path, parameters, {detached: true, stdio: 'ignore'});

      this.setState({launches: this.state.launches + 1})
      protocol.on('close', (code) => {
        if(this.props.isUser) {
          this.TrackActivity(false);
        }
        this.setState({askFeedback: true, launches: 0})
      })
    }
  }


  TrackActivity = (action) => {
    this.props.dispatch(trackUserActivity(action))
  }

  OpenFeedback = () => {
    this.setState({
      askFeedback: true,
      feedbackThankYou: false
    })
  }

  UpdateFeedback = (evt) => {
    this.setState({
      feedback: evt.target.value
    }, function() {
      console.log(this.state.feedback)
    })
  }

  _HandleKeyDown = (evt) => {
    if(this.state.askFeedback) {
      if(evt.key === 'Enter') {
        if(this.state.feedback) {
          this.setState({feedbackThankYou: true, askFeedback: false})
        } else {
          this.setState({feedbackThankYou: false, askFeedback: false})
        }
      }
    } else if(this.state.feedbackThankYou) {
      if(evt.key === 'Enter') {
        this.setState({askFeedback: false, feedbackThankYou: false})
        if(this.state.feedback) {
          this.props.dispatch(sendFeedback(this.state.feedback))
        }
      } else {
        this.setState({askFeedback: true, feedbackThankYou: false})
      }
    }
  }

  ForwardFeedbackButton = () => {
    if(this.state.feedback) {
      this.setState({feedbackThankYou: true, askFeedback: false})
    } else {
      this.setState({feedbackThankYou: false, askFeedback: false})
    }
  }

  SubmitFeedbackButton = () => {
    this.setState({askFeedback: false, feedbackThankYou: false})
    if(this.state.feedback) {
      this.props.dispatch(sendFeedback(this.state.feedback))
    }
  }

  LogOut = () => {
    this.props.dispatch(storeUserInfo(null, null))
    history.push("/");
  }

  componentDidMount() {
    this.MeasureConnectionSpeed();
    this.MeasureCores();
    this.setState({isLoading: false})
    document.addEventListener("keydown", this._HandleKeyDown);
  }

  componentDidUpdate(prevProps) {
    if(this.props.public_ip != null && this.state.distance == 0) {
      this.CalculateDistance(this.props.public_ip)
    }

    if(this.props.resetFeedback) {
      console.log("RESETTING FEEDBACK")
      this.setState({feedback: ''})
      this.props.dispatch(resetFeedback(false))
    }
  }

  render() {
    let internetBox;
    let distanceBox;
    let cpuBox;

    const barHeight = 5

    if(this.state.internetspeed < 10) {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Internet: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}>{this.state.internetspeed} Mbps</span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            Your Internet bandwidth is slow. Try closing streaming apps like Youtube, Netflix, or Spotify.
          </div>
        </div>
    } else if(this.state.internetspeed < 20) {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #f2a20c", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Internet: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}>{this.state.internetspeed} Mbps</span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            Expect a smooth streaming experience, although dips in Internet speed below 10 Mbps could cause low image quality.
          </div>
        </div>
    } else {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #3ce655", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Internet: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}>{this.state.internetspeed} Mbps</span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            Your Internet is fast enough to support high-quality streaming.
          </div>
        </div>
    }

    if(this.state.distance < 250) {
      distanceBox =
        <div>
         <div style = {{background: "none", border: "solid 1px #3ce655", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
         </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            You are close enough to your cloud PC to experience low-latency streaming.
          </div>
        </div>
    } else if(this.state.distance < 500) {
      distanceBox =
        <div>
         <div style = {{background: "none", border: "solid 1px #f2a20c", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
         </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            You may experience slightly higher latency due to your distance from your cloud PC.
          </div>
        </div>
    } else {
      distanceBox =
        <div>
         <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
         </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
          </div>
          <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
            You may experience latency because you are far away from your cloud PC.
          </div>
        </div>
    }

    if(this.state.cores < 4) {
      cpuBox =
      <div>
       <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
       </div>
        <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
          CPU: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.cores} cores </span>
        </div>
        <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
          You need at least three logical CPU cores to run Fractal.
        </div>
      </div>
    } else {
      cpuBox =
      <div>
       <div style = {{background: "none", border: "solid 1px #3ce655", height: 6, width: 6, borderRadius: 3, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 7}}>
       </div>
        <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
          CPU: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.cores} cores </span>
        </div>
        <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
          Your CPU has enough cores to run Fractal.
        </div>
      </div>
    }

    return (
      <div className={styles.container} data-tid="container" style = {{fontFamily: "Maven Pro"}}>
        {
        this.props.os === 'win32'
        ?
        <div style = {{textAlign: 'right', paddingTop: 10, paddingRight: 20}}>
          <div onClick = {this.MinimizeWindow} style = {{display: 'inline', paddingRight: 25, position: 'relative', bottom: 6, zIndex: 4}}>
             <FontAwesomeIcon className = {styles.windowControl} icon={faWindowMinimize} style = {{color: '#999999', height: 10}}/>
          </div>
          <div onClick = {this.CloseWindow} style = {{display: 'inline', position: 'relative', zIndex: 4}}>
             <FontAwesomeIcon className = {styles.windowControl} icon={faTimes} style = {{color: '#999999', height: 16}}/>
          </div>
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
        {
          this.state.askFeedback
          ?
          <div onKeyDown = {this.ForwardFeedback} style = {{position: 'absolute', top: 0, left: 0, width: 900, height: 600, background: '#0B172B', zIndex: 2, textAlign: 'left'}}>
            <div style = {{padding: 150}} className = {styles.feedbackBox}>
              <div style = {{fontSize: 24, color: '#5EC4EB', fontWeight: 'bold'}}>
                How Was Your Experience?
              </div>
              <div style = {{marginTop: 20, fontSize: 15, color: '#5EC4EB', opacity: 0.6}}>
                Our engineers rely on the bugs, feature suggestions, and general feedback you provide to build a better product!
              </div>
              <input type = "text"  value = {this.state.feedback} placeholder = "Type your answer here" onChange = {this.UpdateFeedback} style = {{marginTop: 35, width: 600, background: 'none', border: 'none', borderBottom: 'solid 0.5px #5EC4EB', outline: 'none', padding: '10px 10px 10px 0px', fontSize: 24, color: '#5EC4EB'}}>
              </input>
              {
              this.state.feedback === ""
              ?
              <div>
                <button onClick = {this.ForwardFeedbackButton} className = {styles.feedbackButton} style = {{display: 'inline', width: 100}}>
                  SKIP
                  <FontAwesomeIcon icon = {faCheck} style = {{paddingLeft: 8, height: 14}}/>
                </button>
                <div style = {{display: 'inline', fontSize: 12, marginTop: 40, color: '#5EC4EB', marginLeft: 12}}>
                  press <strong>Enter</strong>
                  <FontAwesomeIcon icon = {faArrowRight} style = {{paddingLeft: 6, height: 10, position: 'relative'}}/>
                </div>
              </div>
              :
              <div>
                <button onClick = {this.ForwardFeedbackButton} className = {styles.feedbackButton} style = {{display: 'inline'}}>
                  OK
                  <FontAwesomeIcon icon = {faCheck} style = {{paddingLeft: 8, height: 14}}/>
                </button>
                <div style = {{display: 'inline', fontSize: 12, marginTop: 40, color: '#5EC4EB', marginLeft: 12}}>
                  press <strong>Enter</strong>
                  <FontAwesomeIcon icon = {faArrowRight} style = {{paddingLeft: 6, height: 10, position: 'relative'}}/>
                </div>
              </div>
              }
            </div>
          </div>
          :
          <div>
          </div>
        }
        {
          this.state.feedbackThankYou
          ?
          <div onKeyDown = {this.SubmitFeedback} style = {{position: 'absolute', top: 0, left: 0, width: 900, height: 600, background: '#0B172B', zIndex: 3, textAlign: 'left'}}>
            <div style = {{padding: '300px 150px'}}>
              <div style = {{width: 900, height: 1, position: 'relative', right: 150, background: '#5EC4EB', opacity: 0.4, marginBottom: 30}}></div>
              <button onClick = {this.SubmitFeedbackButton} className = {styles.feedbackButton} style = {{display: 'inline', width: 120}}>
                SUBMIT
                <FontAwesomeIcon icon = {faCheck} style = {{paddingLeft: 8, height: 14}}/>
              </button>
              <div style = {{display: 'inline', fontSize: 12, marginTop: 40, color: '#5EC4EB', marginLeft: 12}}>
                press <strong>Enter</strong>
                <FontAwesomeIcon icon = {faArrowRight} style = {{paddingLeft: 6, height: 10, position: 'relative', bottom: 1}}/>
              </div>
            </div>
          </div>
          :
          <div>
          </div>
        }
        <div className = {styles.landingHeader}>
          <div className = {styles.landingHeaderLeft}>
            <img src = {Logo} width = "20" height = "20"/>
            <span className = {styles.logoTitle}>Fractal</span>
          </div>
          <div className = {styles.landingHeaderRight}>
            <span className = {styles.headerButton} onClick = {this.OpenFeedback}>Support</span>
            <Popup trigger = {
              <span className = {styles.headerButton}>Settings</span>
            } modal contentStyle = {{width: 300, borderRadius: 5, backgroundColor: "#111111", border: "none", height: 100, padding: 30, textAlign: "center"}}>
              <div style = {{fontWeight: 'bold', fontSize: 20}} className = {styles.blueGradient}><strong>Coming Soon</strong></div>
              <div style = {{fontSize: 12, color: "#D6D6D6", marginTop: 20}}>Toggle bandwidth consumption, image quality, and more.</div>
            </Popup>
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
            <div onClick = {this.LaunchProtocol} className = {styles.bigBox} style = {{position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(0, 0, 0, 0.0), rgba(0,0,0,0.6)), url(" + Car + ")", width: "100%", height: 250, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
              <div style = {{position: 'absolute', bottom: 10, right: 15, fontWeight: 'bold', fontSize: 16}}>
                Launch My Cloud PC
              </div>
            </div>
            <div style = {{display: 'flex', marginTop: 20}}>
              <div style = {{width: '50%', paddingRight: 20, textAlign: 'center'}}>
                <Popup trigger = {
                <div className = {styles.bigBox} style = {{background: "linear-gradient(217.69deg, #363868 0%, rgba(30, 31, 66, 0.5) 101.4%)", borderRadius: 5, padding: 10, minHeight: 110, paddingTop: 30, paddingBottom: 0}}>
                  <img src = {Video} style = {{height: 40}}/>
                  <div style = {{marginTop: 20, fontSize: 14, fontWeight: 'bold'}}>
                    Ultra-Fast Video Upload
                  </div>
                </div>
                } modal contentStyle = {{width: 300, borderRadius: 5, backgroundColor: "#111111", border: "none", height: 100, padding: 30}}>
                  <div style = {{fontWeight: 'bold', fontSize: 20}} className = {styles.blueGradient}><strong>Coming Soon</strong></div>
                  <div style = {{fontSize: 12, color: "#D6D6D6", marginTop: 20}}>Uses proprietary compression algorithms to upload large video files in minutes.</div>
                </Popup>
              </div>
              <div style = {{width: '50%', textAlign: 'center'}}>
                <Popup trigger = {
                  <div className = {styles.bigBox} style = {{background: "linear-gradient(133.09deg, rgba(73, 238, 228, 0.8) 1.86%, rgba(109, 151, 234, 0.8) 100%)", borderRadius: 5, padding: 10, minHeight: 110, paddingTop: 25, paddingBottom: 5}}>
                    <img src = {Folder} style = {{height: 50}}/>
                    <div style = {{marginTop: 15, fontSize: 14, fontWeight: 'bold'}}>
                      File Upload
                    </div>
                  </div>
                } modal contentStyle = {{width: 300, borderRadius: 5, backgroundColor: "#111111", border: "none", height: 100, padding: 30, textAlign: "center"}}>
                  <div style = {{fontWeight: 'bold', fontSize: 20}} className = {styles.blueGradient}><strong>Coming Soon</strong></div>
                  <div style = {{fontSize: 12, color: "#D6D6D6", marginTop: 20}}>Upload an entire disk to your cloud PC.</div>
                </Popup>
              </div>
            </div>
          </div>
          <div className = {styles.statBox} style = {{width: '35%', textAlign: 'left', background: "linear-gradient(217.69deg, #363868 0%, rgba(30, 31, 66, 0.5) 101.4%)", borderRadius: 5, padding: 30, minHeight: 350}}>
            <div style = {{fontWeight: 'bold', fontSize: 20}}>
              Welcome, {this.props.username}
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
            <div style = {{marginTop: 35}}>
              {
                this.state.internetspeed === 0
                ?
                <div>
                  <div style = {{width: 220, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111"}}>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
                    <div style = {{background: "none", border: "solid 1px #5EC4EB", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3, marginRight: 7}}>
                    </div>
                    Checking Internet speed
                  </div>
                </div>
                :
                <div>
                  <div style = {{width: 220, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111", position: "absolute", zIndex: 0}}>
                  </div>
                  <div style = {{width: `${ this.state.internetbar }px`, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#5EC4EB", position: "absolute", zIndex: 1}}>
                  </div>
                  <div style = {{height: 10}}></div>
                  {internetBox}
                </div>
              }
            </div>
            <div style = {{marginTop: 35}}>
              {
                this.state.distance === 0
                ?
                <div>
                  <div style = {{width: 220, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111"}}>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 11, color: "white"}}>
                    <div style = {{background: "none", border: "solid 1px #5EC4EB", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3, marginRight: 7}}>
                    </div>
                    Checking current location
                  </div>
                </div>
                :
                <div>
                  <div style = {{width: 220, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111", position: "absolute", zIndex: 0}}>
                  </div>
                  <div style = {{width: `${ this.state.distancebar }px`, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#5EC4EB", position: "absolute", zIndex: 1}}>
                  </div>
                  <div style = {{height: 10}}></div>
                  {distanceBox}
                </div>
              }
            </div>
            <div style = {{marginTop: 35}}>
              {
                this.state.cores === 0
                ?
                <div>
                  <div style = {{width: 220, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111"}}>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
                    <div style = {{background: "none", border: "solid 1px #5EC4EB", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3, marginRight: 7}}>
                    </div>
                    Scanning CPU
                  </div>
                </div>
                :
                <div>
                  <div style = {{width: 220, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111", position: "absolute", zIndex: 0}}>
                  </div>
                  <div style = {{width: `${ this.state.corebar }px`, height: `${ barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#5EC4EB", position: 'absolute', zIndex: 1}}>
                  </div>
                  <div style = {{height: 10}}></div>
                  {cpuBox}
                </div>
              }
            </div>
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
    resetFeedback: state.counter.resetFeedback,
    isUser: state.counter.isUser,
    os: state.counter.os
  }
}

export default  connect(mapStateToProps)(Counter);
