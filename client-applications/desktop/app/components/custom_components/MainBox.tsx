import React, { Component } from 'react';
import * as geolib from 'geolib';
import { connect } from 'react-redux';
import Popup from "reactjs-popup"
import ToggleButton from 'react-toggle-button'
import Slider from 'react-input-slider';

import styles from '../Counter.css';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCheck, faArrowRight, faCogs, faWindowMaximize, faClock, faKeyboard, faDesktop, faInfoCircle, faPencilAlt,
  faPlus, faCircleNotch, faWrench, faUpload } from '@fortawesome/free-solid-svg-icons'

import Folder from "../../../resources/images/folder.svg";
import Video from "../../../resources/images/video.svg";
import Window from "../../../resources/images/window.svg";
import Speedometer from "../../../resources/images/speedometer.svg";
import Car from "../../../resources/images/car.jpg";
import Scale from "../../../resources/images/scale.svg"

import { resetFeedback, sendFeedback, askFeedback, trackUserActivity, changeWindow, attachDisk, restartPC, vmRestarted, sendLogs } from "../../actions/counter"

class MainBox extends Component {
  constructor(props) {
    super(props)
    this.state = {launches: 0, windowMode: false, mbps: 50, nickname: '', editNickname: -1, diskAttaching: false,
                  launched: false, reattached: false, restartPopup: false, vmRestarting: false, scale: 100}
  }

  TrackActivity = (action) => {
    this.props.dispatch(trackUserActivity(action))
  }

  ExitSettings = () => {
    console.log(this.props.default)
    this.props.dispatch(changeWindow(this.props.default))
  }

  ChangeNickname = (evt) => {
    this.setState({
      nickname: evt.target.value
    })
  }

  NicknameEdit = (index) => {
    this.setState({editNickname: index})
  }

  openRestartPopup = (open) => {
    this.setState({restartPopup: open})
  }

  SendLogs = () => {
    var fs = require("fs");
    var appRootDir = require('electron').remote.app.getAppPath();
    const os       = require('os');

    if (os.platform() === 'darwin' || os.platform() === 'linux') {
      var logs = fs.readFileSync(__dirname + "/../../log.txt").toString();
      var connection_id = parseInt(fs.readFileSync(__dirname + "/../../connection_id.txt").toString());
      this.props.dispatch(sendLogs(connection_id, logs))
    } else if (os.platform() === 'win32') {
      var logs = fs.readFileSync(process.cwd() + "\\protocol\\desktop\\log.txt").toString();
      var connection_id = parseInt(fs.readFileSync(process.cwd() + "\\connection_id.txt").toString());
      this.props.dispatch(sendLogs(connection_id, logs))
    }
  }

  LaunchProtocol = () => {
    if(this.state.reattached && this.props.public_ip && this.props.public_ip != '') {
      this.setState({launched: true})
      if(this.state.launches == 0) {
        this.setState({launches: 1}, function() {
          var child      = require('child_process').spawn;
          var appRootDir = require('electron').remote.app.getAppPath();
          const os       = require('os');

          // check which OS we're on to properly launch the protocol
          if (os.platform() === 'darwin' || os.platform() === 'linux') { // mac & linux
            var path = appRootDir + "/protocol/desktop/FractalClient"
            path = path.replace('/resources/app.asar','');
            path = path.replace('/desktop/app', '/desktop');
          }
          else if (os.platform() === 'win32') { // windows
            var path = process.cwd() + "\\protocol\\desktop\\FractalClient.exe"
          }

          var screenWidth = this.state.windowMode ? window.screen.width : 0
          var screenHeight = this.state.windowMode ? (window.screen.height - 70) : 0

          var parameters = [this.props.public_ip, screenWidth, screenHeight, this.state.mbps]

          if(this.state.launches == 1) {
            this.TrackActivity(true);
          }

          const protocol = child(path, parameters, {detached: true, stdio: 'ignore', windowsHide: true});

          protocol.on('close', (code) => {
            if(this.state.launches == 1) {
              this.TrackActivity(false);
              setTimeout(this.SendLogs, 0 ); // send the logs, async
            }
            this.setState({launches: 0, launched: false, reattached: false, diskAttaching: false})
            this.props.dispatch(askFeedback(true))
          })
        })
      }
    } else {
      this.setState({launched: false})
      this.setState({diskAttaching: true})
      this.props.dispatch(attachDisk())
    }
  }

  toggleWindow = (mode) => {
    const storage = require('electron-json-storage');
    let component = this;

    this.setState({windowMode: !mode}, function() {
      storage.set('window', {windowMode: !mode})
    })
  }

  changeMbps = (mbps) => {
    const storage = require('electron-json-storage');

    this.setState({mbps: parseFloat(mbps.toFixed(2))})
    storage.set('settings', {mbps: mbps})
  }

  changeScale = (scale) => {
    const storage = require('electron-json-storage');
    let component = this;

    this.setState({scale: scale}, function() {
      storage.set('scale', {scale: scale})
    })
  }


  OpenDashboard = () => {
    const {shell} = require('electron')
    shell.openExternal('https://www.fractalcomputers.com/dashboard')
  }

  RestartPC = () => {
    this.setState({restartPopup: false, vmRestarting: true}, function() {
      console.log("STATE CHANGED ON RESTART")
      console.log(this.state)
    })
    this.props.dispatch(restartPC())
  }

  componentDidMount() {
    const storage = require('electron-json-storage');
    let component = this;

    storage.get('settings', function(error, data) {
      if (error) throw error;
      if(data.mbps) {
        component.setState({mbps: data.mbps})
      }
    });

    storage.get('window', function(error, data) {
      if (error) throw error;
      component.setState({windowMode: data.windowMode})
    });

    storage.get('scale', function(error, data) {
      if (error) throw error;
      component.setState({scale: data.scale})
    });
  }

  componentDidUpdate(prevProps) {
    console.log(prevProps.restart_attempts)
    console.log(this.props.restart_attempts)
    if(prevProps.attach_attempts != this.props.attach_attempts && this.state.diskAttaching) {
      this.setState({diskAttaching: false, reattached: true})
    }

    if(prevProps.restart_attempts != this.props.restart_attempts && this.state.vmRestarting) {
      console.log("VM DONE RESTARTING!")
      this.setState({vmRestarting: false})
    }
  }

  render() {
    if(this.props.currentWindow === 'main') {
      return (
        <div>
          {
          this.props.account_locked
          ?
          <div onClick = {this.OpenDashboard} className = {styles.pointerOnHover} style = {{boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(255, 255, 255, 0.8), rgba(255, 255, 255, 0.8)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div style = {{marginTop: 30, marginBottom: 20, width: 350}}>
                Oops! Your Free Trial Has Expired
              </div>
              <div style = {{marginTop: 10, color: '#CCCCCC', fontSize: 12, lineHeight: 1.4}}>
                Please provide your payment details in order to access your cloud PC.
              </div>
            </div>
          </div>
          :
          (
          this.state.vmRestarting
          ?
          <div style = {{boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(255, 255, 255, 0.8), rgba(255, 255, 255, 0.8)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "#111111", height: 30}}/>
              </div>
              <div style = {{marginTop: 10, color: '#111111', fontSize: 14, lineHeight: 1.4}}>
                Restarting
              </div>
            </div>
          </div>
          :
          (
          this.props.fetchStatus
          ?
          (
          this.props.disk != ''
          ?
          (
          !this.state.diskAttaching
          ?
          (
          this.state.launched
          ?
          <div style = {{boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(255, 255, 255, 0.8), rgba(255, 255, 255, 0.8)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "#111111", height: 30}}/>
              </div>
              <div style = {{marginTop: 10, color: '#111111', fontSize: 14, lineHeight: 1.4}}>
                Streaming
              </div>
            </div>
          </div>
          :
          <div onClick = {this.LaunchProtocol} className = {styles.bigBox} style = {{position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(0, 0, 0, 0.0), rgba(0,0,0,0.0)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{position: 'absolute', bottom: 10, right: 15, fontWeight: 'bold', fontSize: 16}}>
              Launch My Cloud PC
            </div>
          </div>
          )
          :
          <div style = {{boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(255, 255, 255, 0.8), rgba(255, 255, 255, 0.8)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "#111111", height: 30}}/>
              </div>
              <div style = {{marginTop: 10, color: '#111111', fontSize: 14, lineHeight: 1.4}}>
                Booting your cloud PC (this could take a few minutes)
              </div>
            </div>
          </div>
          )
          :
          <div onClick = {this.OpenDashboard} className = {styles.pointerOnHover} style = {{boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(255, 255, 255, 0.85), rgba(255, 255, 255, 0.85)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon = {faPlus} style = {{height: 30, color: '#111111'}}/>
              </div>
              <div style = {{marginTop: 25, color: "#111111"}}>
                <span className = {styles.blueGradient}>Create My Cloud PC</span>
              </div>
              <div style = {{marginTop: 20, color: '#333333', fontSize: 14, lineHeight: 1.4}}>
                Transform your computer into a GPU-powered workstation.
              </div>
            </div>
          </div>
          )
          :
          <div style = {{boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(255, 255, 255, 0.8), rgba(255,255,255,0.8)), url(" + Car + ")", width: "100%", height: 275, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "#111111", height: 30}}/>
              </div>
              <div style = {{marginTop: 10, color: '#333333', fontSize: 16, lineHeight: 1.4}}>
                Loading your account
              </div>
            </div>
          </div>
          )
          )
          }
          <div style = {{display: 'flex', marginTop: 20}}>
            <div style = {{width: '50%', paddingRight: 20, textAlign: 'center'}}>
              {
              this.props.public_ip && this.props.public_ip !== ''
              ?
              <Popup open = {this.state.restartPopup}
              trigger = {
              <div className = {styles.bigBox} onClick = {() => this.openRestartPopup(true)} style = {{background: "white", borderRadius: 5, padding: 10, minHeight: 90, paddingTop: 20, paddingBottom: 0}}>
                <FontAwesomeIcon icon = {faWrench} style = {{height: 40, color: "#111111"}}/>
                <div style = {{marginTop: 5, fontSize: 14, fontWeight: 'bold', color: "#111111"}}>
                  Troubleshoot
                </div>
              </div>
              } modal contentStyle = {{width: 350, color: "#111111", borderRadius: 5, backgroundColor: "white", border: "none", height: 170, padding: 30}}>
                <div style = {{fontWeight: 'bold', fontSize: 20}}>Have Trouble Connecting?</div>
                <div style = {{fontSize: 14, lineHeight: 1.4, width: 300, margin: "20px auto"}}>Restarting your cloud PC could help. Note that restarting will close any files or applications you have open.</div>
                {
                !this.state.vmRestarting
                ?
                <button  onClick = {this.RestartPC} type = "button" className = {styles.signupButton} id = "signup-button" style = {{width: 300, marginLeft: 0, marginTop: 20, fontFamily: "Maven Pro", fontWeight: 'bold'}}>
                  Restart Cloud PC
                </button>
                :
                <button type = "button" className = {styles.signupButton} id = "signup-button" style = {{width: 300, marginLeft: 0, marginTop: 20, fontFamily: "Maven Pro", fontWeight: 'bold'}}>
                  <FontAwesomeIcon icon = {faCircleNotch} spin style = {{color: '#1ba8e0', height: 12}}/>
                </button>
                }
              </Popup>
              :
              <Popup
              trigger = {
              <div className = {styles.bigBox} style = {{background: "white", borderRadius: 5, padding: 10, minHeight: 90, paddingTop: 20, paddingBottom: 0}}>
                <FontAwesomeIcon icon = {faWrench} style = {{height: 40, color: "#111111"}}/>
                <div style = {{marginTop: 5, fontSize: 14, fontWeight: 'bold', color: "#111111"}}>
                  Troubleshoot
                </div>
              </div>
              } modal contentStyle = {{width: 350, color: "#111111", borderRadius: 5, backgroundColor: "white", border: "none", height: 100, padding: 30}}>
                <div style = {{fontWeight: 'bold', fontSize: 20}}>Have Trouble Connecting?</div>
                <div style = {{fontSize: 14, lineHeight: 1.4, width: 300, margin: "20px auto"}}>Boot your cloud PC first by selecting the "Launch My Cloud PC" button.</div>
              </Popup>
              }
            </div>
            <div style = {{width: '50%', textAlign: 'center'}}>
              <Popup trigger = {
              <div className = {styles.bigBox} style = {{background: "linear-gradient(133.09deg, rgba(73, 238, 228, 0.8) 1.86%, rgba(109, 151, 234, 0.8) 100%)", borderRadius: 5, padding: 10, minHeight: 90, paddingTop: 20, paddingBottom: 0}}>
                <FontAwesomeIcon icon = {faUpload} style = {{height: 40, color: "white"}}/>
                <div style = {{marginTop: 5, fontSize: 14, fontWeight: 'bold', color: "white"}}>
                  File Upload
                </div>
              </div>
              } modal contentStyle = {{width: 300, borderRadius: 5, backgroundColor: "white", border: "none", height: 100, padding: 20, textAlign: "center"}}>
                <div style = {{fontWeight: 'bold', fontSize: 22, marginTop: 10}} className = {styles.blueGradient}><strong>Coming Soon</strong></div>
                <div style = {{fontSize: 14, lineHeight: 1.4, color: "#111111", marginTop: 20}}>Upload any folder to your cloud PC.</div>
              </Popup>
            </div>
          </div>
        </div>
      )
    } else if(this.props.currentWindow === 'studios') {
      return(
      <div>
      <div style = {{position: 'relative', width: '100%', height: 410, borderRadius: 5, boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', background: 'linear-gradient(205.96deg, #363868 0%, #1E1F42 101.4%)', overflowY: 'scroll'}}>
        <div style = {{backgroundColor: '#161936', padding: "20px 30px", color: 'white', fontSize: 18, fontWeight: 'bold', borderRadius: "5px 0px 0px 0px", fontFamily: 'Maven Pro'}}>
          <div style = {{float: 'left', display: 'inline'}}>
            <FontAwesomeIcon icon = {faKeyboard} style = {{height: 15, paddingRight: 4, position: 'relative', bottom: 1}}/> My Computers
          </div>
          <div style = {{float: 'right', display: 'inline'}}>
            <Popup trigger = {
            <span>
              <FontAwesomeIcon className = {styles.pointerOnHover} icon = {faInfoCircle} style = {{height: 20, color: '#AAAAAA'}}/>
             </span>
            } modal contentStyle = {{width: 400, borderRadius: 5, backgroundColor: "#111111", border: "none", height: 100, padding: 30, textAlign: "center"}}>
              <div style = {{fontWeight: 'bold', fontSize: 20}} className = {styles.blueGradient}><strong>How It Works</strong></div>
              <div style = {{fontSize: 12, color: "#CCCCCC", marginTop: 20, lineHeight: 1.5}}>
                This dashboard contains a list of all computers that you've installed Fractal onto. You can start a remote connection to any of these
                computers, as long `as they are turned on.
              </div>
            </Popup>
          </div>
          <div style = {{display: 'block', height: 20}}>
          </div>
        </div>
        {this.props.computers.map((value, index) => {
          var defaultNickname = value.nickname
          return (
            <div style = {{padding: '30px 30px', borderBottom: 'solid 0.5px #161936'}}>
              <div style = {{display: 'flex'}}>
                <div style = {{width: '69%'}}>
                  {
                  this.state.editNickname === index
                  ?
                  <div className = {styles.nicknameContainer} style = {{color: 'white', fontSize: 14, fontWeight: 'bold'}}>
                    <FontAwesomeIcon className = {styles.pointerOnHover} onClick = {() => this.NicknameEdit(-1)} icon = {faCheck} style = {{height: 14, marginRight: 12, position: 'relative', width: 16}}/>
                    <input defaultValue = {defaultNickname} type = "text" placeholder = "Rename Computer" onChange = {this.ChangeNickname} style = {{background: 'none', border: 'solid 0.5px #161936', padding: '6px 10px', borderRadius: 5, outline: 'none', color: 'white'}}/>
                    <div style = {{height: 12}}></div>
                  </div>
                  :
                  <div>
                    <div style = {{color: 'white', fontSize: 14, fontWeight: 'bold'}}>
                      <FontAwesomeIcon className = {styles.pointerOnHover} onClick = {() => this.NicknameEdit(index)} icon = {faPencilAlt} style = {{height: 14, marginRight: 12, position: 'relative', width: 16}}/>
                      {value.nickname}
                    </div>
                    <div style = {{fontSize: 12, color: '#D6D6D6', marginTop: 10, marginLeft: 28}}>
                      {value.location}
                    </div>
                  </div>
                  }
                </div>
                <div style = {{width: '31%'}}>
                  <div style = {{float: 'right'}}>
                    {
                    value.id === this.props.id
                    ?
                    <div style = {{width: 110, textAlign: 'center', fontSize: 12, fontWeight: 'bold', border: 'none', borderRadius: 5, paddingTop: 7, paddingBottom: 7, background: '#161936', color: "#D6D6D6"}}>
                      This Computer
                    </div>
                    :
                    <div className = {styles.pointerOnHover} style = {{width: 110, textAlign: 'center', fontSize: 12, fontWeight: 'bold', border: 'none', borderRadius: 5, paddingTop: 7, paddingBottom: 7, background: '#5EC4EB', color: "white"}}>
                      Connect
                    </div>
                    }
                  </div>
                </div>
              </div>
            </div>
          )
        })}
      </div>
      </div>
      )
    } else if (this.props.currentWindow === 'settings') {
      return (
        <div className = {styles.settingsContainer}>
          <div style = {{position: 'relative', width: 800, height: 410, borderRadius: 5, boxShadow: '0px 4px 20px rgba(0, 0, 0, 0.2)', background: 'white', overflowY: 'scroll'}}>
            <div style = {{backgroundColor: '#EFEFEF', padding: "20px 30px", color: '#111111', fontSize: 18, fontWeight: 'bold', borderRadius: "5px 0px 0px 0px", fontFamily: 'Maven Pro'}}>
              <FontAwesomeIcon icon = {faCogs} style = {{height: 15, paddingRight: 4, position: 'relative', bottom: 1}}/> Settings
            </div>
            <div style = {{padding: '30px 30px', borderBottom: 'solid 0.5px #EFEFEF'}}>
              <div style = {{display: 'flex'}}>
                <div style = {{width: '75%'}}>
                  <div style = {{color: '#111111', fontSize: 16, fontWeight: 'bold'}}>
                    <img src = {Window} style = {{color: '#111111', height: 14, marginRight: 12, position: 'relative', top: 2, width: 16}}/>
                    Windowed Mode
                  </div>
                  <div style = {{fontSize: 13, color: '#333333', marginTop: 10, marginLeft: 28, lineHeight: 1.4}}>
                    When activated, a titlebar will appear at the top of your cloud PC, so you can adjust your cloud PC's
                    position on your screen.
                  </div>
                </div>
                <div style = {{width: '25%'}}>
                  <div style = {{float: 'right'}}>
                    <ToggleButton
                      value = {this.state.windowMode}
                      onToggle = {this.toggleWindow}
                      colors={{
                        active: {
                          base: '#5EC4EB',
                        },
                        inactive: {
                          base: '#161936'
                        }
                      }}/>
                  </div>
                </div>
              </div>
            </div>
            <div style = {{padding: '30px 30px', borderBottom: 'solid 0.5px #EFEFEF'}}>
              <div style = {{display: 'flex'}}>
                <div style = {{width: '75%'}}>
                  <div style = {{color: '#111111', fontSize: 16, fontWeight: 'bold'}}>
                    <img src = {Scale} style = {{color: '#111111', height: 14, marginRight: 12, position: 'relative', top: 2, width: 16}}/>
                    Scaling Factor
                  </div>
                  <div style = {{fontSize: 13, color: '#333333', marginTop: 10, marginLeft: 28, lineHeight: 1.4}}>
                    Increase or decrease your scaling factor to change the width and height of icons on your cloud PC.
                  </div>
                </div>
                <div style = {{width: '25%'}}>
                  <div style = {{float: 'right'}}>
                    {
                    this.state.scale === 100
                    ?
                    <select id="resolution-select" style = {{padding: 4, outline: 'none'}}>
                      <option value="100%" selected onClick = {() => this.setScale(100)}>100%</option>
                      <option value="150%" onClick = {() => this.setScale(150)}>150%</option>
                    </select>
                    :
                    <select id="resolution-select" style = {{padding: 4, outline: 'none'}}>
                      <option value="100%" onClick = {() => this.setScale(100)}>100%</option>
                      <option value="150%" selected onClick = {() => this.setScale(150)}>150%</option>
                    </select>
                    }
                  </div>
                </div>
              </div>
            </div>
            <div style = {{padding: '30px 30px', borderBottom: 'solid 0.5px #EFEFEF'}}>
              <div style = {{display: 'flex'}}>
                <div style = {{width: '75%'}}>
                  <div style = {{color: '#111111', fontSize: 16, fontWeight: 'bold'}}>
                    <img src = {Speedometer} style = {{color: '#111111', height: 14, marginRight: 12, position: 'relative', top: 2, width: 16}}/>
                    Maximum Bandwidth
                  </div>
                  <div style = {{fontSize: 13, color: '#333333', marginTop: 10, marginLeft: 28, lineHeight: 1.4}}>
                    Toggle the maximum bandwidth (Mbps) that Fractal consumes. We recommend adjusting
                    this setting only if you are also running other, bandwidth-consuming apps.
                  </div>
                </div>
                <div style = {{width: '25%'}}>
                  <div style = {{float: 'right'}}>
                    <Slider
                      axis="x"
                      xstep={5}
                      xmin={10}
                      xmax={50}
                      x={this.state.mbps}
                      onChange={({ x }) => this.changeMbps(x)}
                      styles={{
                        track: {
                          backgroundColor: '#161936',
                          height: 5,
                          width: 100
                        },
                        active: {
                          backgroundColor: '#5EC4EB',
                          height: 5,
                        },
                        thumb: {
                          height: 10,
                          width: 10
                        }
                      }}
                    />
                  </div><br/>
                  <div style = {{fontSize: 11, color: '#333333', float: 'right', marginTop: 5}}>
                    {this.state.mbps} Mbps
                  </div>
                </div>
              </div>
            </div>
            <div style = {{padding: '30px 30px', marginBottom: 30}}>
              <div style = {{float: 'right'}}>
                <button onClick = {this.ExitSettings} className = {styles.signupButton} style = {{borderRadius: 5, width: 140}}>
                  Save &amp; Exit
                </button>
              </div>
            </div>
          </div>
        </div>
      )
    }
  }
}

function mapStateToProps(state) {
  console.log(state)
  return {
    username: state.counter.username,
    isUser: state.counter.isUser,
    public_ip: state.counter.public_ip,
    ipInfo: state.counter.ipInfo,
    computers: state.counter.computers,
    fetchStatus: state.counter.fetchStatus,
    disk: state.counter.disk,
    attach_attempts: state.counter.attach_attempts,
    account_locked: state.counter.account_locked,
    promo_code: state.counter.promo_code,
    restart_status: state.counter.restart_status,
    restart_attempts: state.counter.restart_attempts
  }
}

export default connect(mapStateToProps)(MainBox);
