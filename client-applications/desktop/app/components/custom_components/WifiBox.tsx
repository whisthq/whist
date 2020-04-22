import React, { Component } from 'react'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

class WifiBox extends Component {
  constructor(props) {
    super(props)
    this.state = {internetspeed: 0, internetbar: 50}
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

  componentDidMount() {
    this.MeasureConnectionSpeed();
  }


  render() {
    let internetBox;
    
    if(this.state.internetspeed < 10) {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            {this.state.internetspeed} Mbps Internet
          </div>
          <div style = {{marginTop: 8, fontSize: 12, color: "#333333", lineHeight: 1.4, paddingLeft: 20}}>
            Your Internet bandwidth is slow. Try closing streaming apps like Youtube, Netflix, or Spotify.
          </div>
        </div>
    } else if(this.state.internetspeed < 20) {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #f2a20c", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            {this.state.internetspeed} Mbps Internet
          </div>
          <div style = {{marginTop: 8, fontSize: 12, color: "#333333", lineHeight: 1.4, paddingLeft: 20}}>
            Expect a smooth streaming experience with occassional image quality drops.
          </div>
        </div>
    } else {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #14a329", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            {this.state.internetspeed} Mbps Internet
          </div>
          <div style = {{marginTop: 8, fontSize: 12, color: "#333333", lineHeight: 1.4, paddingLeft: 20}}>
            Your Internet is fast enough to support high-quality streaming.
          </div>
        </div>
    }

    return (
      <div style = {{marginTop: 15}}>
        {
          this.state.internetspeed === 0
          ?
          <div style = {{marginTop: 25, position: 'relative', right: 5}}>
            <FontAwesomeIcon icon = {faCircleNotch} spin style = {{color: "#5EC4EB", height: 10, display: 'inline', marginRight: 9}}/>
            <div style = {{marginTop: 8, fontSize: 12, color: "#333333", display: 'inline'}}>
              Checking Internet speed
            </div>
          </div>
          :
          <div>
            <div style = {{height: 10}}></div>
            {internetBox}
          </div>
        }
      </div>
    )
  }
}

export default WifiBox