import React, { Component } from 'react';
import * as geolib from 'geolib';
import { connect } from 'react-redux';
import Popup from "reactjs-popup"
import ToggleButton from 'react-toggle-button'
import Slider from 'react-input-slider';

import styles from '../Counter.css';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCheck, faArrowRight, faCogs, faWindowMaximize, faClock, faKeyboard, faDesktop, faInfoCircle, faPencilAlt, faPlus, faCircleNotch } from '@fortawesome/free-solid-svg-icons'

import Folder from "../../../resources/images/folder.svg";
import Video from "../../../resources/images/video.svg";
import Window from "../../../resources/images/window.svg";
import Speedometer from "../../../resources/images/speedometer.svg";
import Car from "../../../resources/images/car.jpg";

import { resetFeedback, sendFeedback, askFeedback, trackUserActivity, changeWindow, sendLogs } from "../../actions/counter"

class MainBox extends Component {
  constructor(props) {
    super(props)
    this.state = {launches: 0, windowMode: false, mbps: 50, nickname: '', editNickname: -1}
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

  LaunchProtocol = () => {
    if(this.state.launches == 0) {
      this.setState({launches: this.state.launches + 1}, function() {
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

        var screenWidth = this.state.windowMode ? window.screen.width : 0
        var screenHeight = this.state.windowMode ? (window.screen.height - 70) : 0

        var parameters = [this.props.public_ip, 123, screenWidth, screenHeight, this.state.mbps]

        if(this.props.isUser && this.state.launches == 1) {
          this.TrackActivity(true);
        }
        const protocol = child(path, parameters, {detached: true, stdio: 'ignore'});

        protocol.on('close', (code) => {
          if(this.props.isUser && this.state.launches == 1) {
            this.TrackActivity(false);
            // READ LOGS AS STRING HERE
            // var logs = 'SOME STRING'
            this.props.dispatch(sendLogs(logs))
          }
          this.setState({launches: 0})
          this.props.dispatch(askFeedback(true))
        })
      })
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

  OpenDashboard = () => {
    const {shell} = require('electron')
    shell.openExternal('https://www.fractalcomputers.com/dashboard')
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
  }

  componentDidUpdate(prevProps) {
    console.log(this.props)
  }

  render() {
    if(this.props.currentWindow === 'main') {
      return (
        <div>
          {
          this.props.fetchStatus
          ?
          (
          this.props.public_ip != ''
          ?
          <div onClick = {this.LaunchProtocol} className = {styles.bigBox} style = {{position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(0, 0, 0, 0.0), rgba(0,0,0,0.6)), url(" + Car + ")", width: "100%", height: 250, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{position: 'absolute', bottom: 10, right: 15, fontWeight: 'bold', fontSize: 16}}>
              Launch My Cloud PC
            </div>
          </div>
          :
          <div onClick = {this.OpenDashboard} className = {styles.pointerOnHover} style = {{boxShadow: '0px 4px 30px rgba(0, 0, 0, 0.35)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(0, 0, 0, 0.9), rgba(0,0,0,0.9)), url(" + Car + ")", width: "100%", height: 250, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon = {faPlus} style = {{height: 30, color: 'white'}}/>
              </div>
              <div style = {{marginTop: 30}}>
                <span className = {styles.blueGradient}>Create My Cloud PC</span>
              </div>
              <div style = {{marginTop: 10, color: '#CCCCCC', fontSize: 12, lineHeight: 1.4}}>
                Transform your computer into a GPU-powered workstation.
              </div>
            </div>
          </div>
          )
          :
          <div style = {{boxShadow: '0px 4px 30px rgba(0, 0, 0, 0.35)', position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(0, 0, 0, 0.9), rgba(0,0,0,0.9)), url(" + Car + ")", width: "100%", height: 250, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
            <div style = {{textAlign: 'center', position: 'absolute', top: '50%', left: '50%', transform: 'translate(-50%, -50%)', fontWeight: 'bold', fontSize: 20}}>
              <div>
                <FontAwesomeIcon icon={faCircleNotch} spin style = {{color: "white", height: 30}}/>
              </div>
              <div style = {{marginTop: 10, color: '#CCCCCC', fontSize: 12, lineHeight: 1.4}}>
                Loading your account
              </div>
            </div>
          </div>
          }
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
      )
    } else if(this.props.currentWindow === 'studios') {
      return(
      <div>
      <div style = {{position: 'relative', width: '100%', height: 410, borderRadius: 5, boxShadow: '0px 4px 30px rgba(0, 0, 0, 0.35)', background: 'linear-gradient(205.96deg, #363868 0%, #1E1F42 101.4%)', overflowY: 'scroll'}}>
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
          <div style = {{position: 'relative', width: 750, height: 410, borderRadius: 5, boxShadow: '0px 4px 30px rgba(0, 0, 0, 0.35)', background: 'linear-gradient(205.96deg, #363868 0%, #1E1F42 101.4%)', overflowY: 'scroll'}}>
            <div style = {{backgroundColor: '#161936', padding: "20px 30px", color: 'white', fontSize: 18, fontWeight: 'bold', borderRadius: "5px 0px 0px 0px", fontFamily: 'Maven Pro'}}>
              <FontAwesomeIcon icon = {faCogs} style = {{height: 15, paddingRight: 4, position: 'relative', bottom: 1}}/> Settings
            </div>
            <div style = {{padding: '30px 30px', borderBottom: 'solid 0.5px #161936'}}>
              <div style = {{display: 'flex'}}>
                <div style = {{width: '75%'}}>
                  <div style = {{color: 'white', fontSize: 14, fontWeight: 'bold'}}>
                    <img src = {Window} style = {{height: 14, marginRight: 12, position: 'relative', top: 2, width: 16}}/>
                    Windowed Mode
                  </div>
                  <div style = {{fontSize: 12, color: '#D6D6D6', marginTop: 10, marginLeft: 28}}>
                    When activated, a titlebar will appear at the top of your cloud PC, so you can adjust your cloud PC's 
                    position on your screen. When turned off, your cloud PC will occupy your entire screen.
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
            <div style = {{padding: '30px 30px', borderBottom: 'solid 0.5px #161936'}}>
              <div style = {{display: 'flex'}}>
                <div style = {{width: '75%'}}>
                  <div style = {{color: 'white', fontSize: 14, fontWeight: 'bold'}}>
                    <img src = {Speedometer} style = {{height: 14, marginRight: 12, position: 'relative', top: 2, width: 16}}/>
                    Maximum Bandwidth
                  </div>
                  <div style = {{fontSize: 12, color: '#D6D6D6', marginTop: 10, marginLeft: 28}}>
                    Toggle the maximum bandwidth (Mbps) that Fractal consumes. We recommend adjusting 
                    this setting only if you are simultaneously running other, bandwidth-consuming apps.
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
                  <div style = {{fontSize: 11, color: '#D1D1D1', float: 'right', marginTop: 5}}>
                    {this.state.mbps} Mbps
                  </div>
                </div>
              </div>
            </div>
            <div style = {{padding: '30px 30px'}}>
              <div style = {{float: 'right'}}>
                <button onClick = {this.ExitSettings} className = {styles.loginButton} style = {{borderRadius: 5, width: 140}}>
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
  return {
    username: state.counter.username,
    isUser: state.counter.isUser,
    public_ip: state.counter.public_ip,
    ipInfo: state.counter.ipInfo,
    computers: state.counter.computers,
    fetchStatus: state.counter.fetchStatus
  }
}

export default connect(mapStateToProps)(MainBox);