import React, {Component} from 'react';
import { Link } from 'react-router-dom';
import styles from './Counter.css';
import routes from '../constants/routes.json';
import Row from 'react-bootstrap/Row'
import Col from 'react-bootstrap/Col'

import Titlebar from 'react-electron-titlebar';
import Logo from "../../resources/images/logo.svg";
import Car from "../../resources/images/car.jpg";
import Folder from "../../resources/images/folder.svg";
import Video from "../../resources/images/video.svg";
import Spinner from "../../resources/images/spinner.svg";

import { Offline, Online } from "react-detect-offline";
import * as geolib from 'geolib';


class Counter extends Component {
  constructor(props) {
    super(props)
    this.state = {username: '', internetspeed: 0, distance: 0, internetbar: 50, distancebar: 50, cores: 0, corebar: 40}
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
        component.setState({internetbar: Math.min(Math.max(speedMbps, 50), 190)})
    }
    
    startTime = (new Date()).getTime();
    var cacheBuster = "?nnn=" + startTime;
    download.src = imageAddr + cacheBuster;
  }

  MeasureDistance = () => {
    const localIpUrl = require('local-ip-url');
    const iplocation = require("iplocation").default; 

    var local_ip = localIpUrl('private', 'ipv4')

    iplocation(local_ip)
       .then((res) => {
         var dist = geolib.getDistance(
         { latitude: 38.676233, longitude: -78.156443 },
         { latitude: res.latitude, longitude: res.longitude }
        );
         dist = dist / 1609.34 + 1
         this.setState({distance: Math.round(dist)})
         dist = 190 * (dist / 1000)
         this.setState({distancebar: Math.min(Math.max(dist, 80), 190)})
    })
    .catch(err => {
      console.log(err)
    });
  }

  MeasureCores = () => {
    const si = require('systeminformation')
    si.cpu().then(data => {
      console.log(data.cores)
      this.setState({cores: data.cores})
      console.log(Math.min(Math.max(40, 190 * (data.cores / 32)), 190))
      this.setState({corebar: Math.min(Math.max(40, 190 * (data.cores / 32)), 190)})
    })
  }

  componentDidMount() {
    this.MeasureConnectionSpeed();
    this.MeasureDistance();
    this.MeasureCores();
  }

  render() {
    return (
      <div className={styles.container} data-tid="container" style = {{fontFamily: "Maven Pro"}}>
        <div>
        <Titlebar backgroundColor="#000000"/>
        </div>
        <div className = {styles.landingHeader}>
          <div className = {styles.landingHeaderLeft}>
            <img src = {Logo} width = "20" height = "20"/>
            <span className = {styles.logoTitle}>Fractal</span>
          </div>
          <div className = {styles.landingHeaderRight}>
            <span className = {styles.headerButton}>Settings</span> 
            <span className = {styles.headerButton}>Refer a Friend</span> 
            <Link to={routes.HOME}>
               <button type = "button" className = {styles.signupButton} id = "signup-button" style = {{marginLeft: 25, fontFamily: "Maven Pro", fontWeight: 'bold'}}>Sign Out</button>
            </Link>
          </div>
        </div>
        <div style = {{display: 'flex', padding: '20px 75px' }}>
          <div style = {{width: '70%', textAlign: 'left', paddingRight: 20}}>
            <div style = {{position: 'relative', backgroundImage: "linear-gradient(to bottom, rgba(0, 0, 0, 0.0), rgba(0,0,0,0.6)), url(" + Car + ")", width: "100%", height: 250, backgroundSize: "cover", backgroundRepeat: "no-repeat", backgroundPosition: "center", borderRadius: 5}}>
              <div style = {{position: 'absolute', bottom: 10, right: 15, fontWeight: 'bold', fontSize: 16}}>
                Launch My Cloud PC
              </div>
            </div>
            <div style = {{display: 'flex', marginTop: 20}}>
              <div style = {{width: '50%', paddingRight: 20, textAlign: 'center'}}>
                <div style = {{background: "linear-gradient(217.69deg, #363868 0%, rgba(30, 31, 66, 0.5) 101.4%)", borderRadius: 5, padding: 10, minHeight: 110, paddingTop: 30, paddingBottom: 0}}>
                  <img src = {Video} style = {{height: 40}}/>
                  <div style = {{marginTop: 20, fontSize: 14, fontWeight: 'bold'}}>
                    Ultra-Fast Video Upload
                  </div>
                </div>
              </div>
              <div style = {{width: '50%', textAlign: 'center'}}>
                <div style = {{background: "linear-gradient(133.09deg, rgba(73, 238, 228, 0.8) 1.86%, rgba(109, 151, 234, 0.8) 100%)", borderRadius: 5, padding: 10, minHeight: 110, paddingTop: 25, paddingBottom: 5}}>
                  <img src = {Folder} style = {{height: 50}}/>
                  <div style = {{marginTop: 15, fontSize: 14, fontWeight: 'bold'}}>
                    File Upload
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div style = {{width: '30%', textAlign: 'left', background: "linear-gradient(217.69deg, #363868 0%, rgba(30, 31, 66, 0.5) 101.4%)", borderRadius: 5, padding: 30, minHeight: 350}}>
            <div style = {{fontWeight: 'bold', fontSize: 20}}>
              Welcome, Ming
            </div>
            <div style = {{marginTop: 10, display: "inline-block"}}>
              <Online>
                <div style = {{background: "#3ce655", height: 8, width: 8, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5}}>
                </div>
                <div style = {{display: "inline", float: "left", marginLeft: 5, fontSize: 13, color: "#D6D6D6"}}>
                  Online
                </div>
              </Online>
              <Offline>
                <div style = {{background: "#cf3e2b", height: 8, width: 8, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5}}>
                </div>
                <div style = {{display: "inline", float: "left", marginLeft: 5, fontSize: 13, color: "#D6D6D6"}}>
                  Offline
                </div>
              </Offline>
            </div>
            <div style = {{marginTop: 35}}>
              {
                this.state.internetspeed === 0
                ?
                <div>
                  <div style = {{width: 190, height: 6, borderRadius: 3, background: "#999999"}}>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
                    Checking Internet speed
                  </div>
                </div>
                :
                <div>
                  <div style = {{width: `${ this.state.internetbar }px`, height: 6, borderRadius: "0px 3px 3px 0px", background: "linear-gradient(116.54deg, #5EC4EB 0%, #D75EEB 100%)"}}>
                  </div>
                  <div style = {{marginTop: 5, fontSize: 13, fontWeight: "bold"}}>
                    Internet: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}>{this.state.internetspeed} Mbps</span>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 10, color: "#D6D6D6"}}>
                    Your Internet is fast enough to support latency-free streaming.
                  </div>
                </div>
              }
            </div>
            <div style = {{marginTop: 35}}>
              {
                this.state.distance === 0
                ?
                <div>
                  <div style = {{width: 190, height: 6, borderRadius: 3, background: "#999999"}}>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
                    Checking current location
                  </div>
                </div>
                :
                <div>
                  <div style = {{width: `${ this.state.distancebar }px`, height: 6, borderRadius: "0px 3px 3px 0px", background: "linear-gradient(116.54deg, #5EC4EB 0%, #D75EEB 100%)"}}>
                  </div>
                  <div style = {{marginTop: 5, fontSize: 13, fontWeight: "bold"}}>
                    Cloud PC Distance: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.distance} mi </span>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 10, color: "#D6D6D6"}}>
                    You are close enough to your cloud PC to experience low-latency streaming.
                  </div>
                </div>
              }
            </div>
            <div style = {{marginTop: 35}}>
              {
                this.state.cores === 0
                ?
                <div>
                  <div style = {{width: 190, height: 6, borderRadius: 3, background: "#999999"}}>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
                    Scanning CPU
                  </div>
                </div>
                :
                <div>
                  <div style = {{width: `${ this.state.corebar }px`, height: 6, borderRadius: "0px 3px 3px 0px", background: "linear-gradient(116.54deg, #5EC4EB 0%, #D75EEB 100%)"}}>
                  </div>
                  <div style = {{marginTop: 5, fontSize: 13, fontWeight: "bold"}}>
                    CPU: <span style = {{color: "#5EC4EB", fontWeight: "bold"}}> {this.state.cores} cores </span>
                  </div>
                  <div style = {{marginTop: 8, fontSize: 10, color: "#D6D6D6"}}>
                    Your CPU has enough cores to run Fractal.
                  </div>
                </div>
              }
            </div>
          </div>
        </div>
      </div>
    );
  }
}

export default Counter;
