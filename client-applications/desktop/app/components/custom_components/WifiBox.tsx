import React, { Component } from 'react'

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

    return (
      <div style = {{marginTop: 35}}>
        {
          this.state.internetspeed === 0
          ?
          <div>
            <div style = {{width: 220, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111"}}>
            </div>
            <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
              <div style = {{background: "none", border: "solid 1px #5EC4EB", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3, marginRight: 7}}>
              </div>
              Checking Internet speed
            </div>
          </div>
          :
          <div>
            <div style = {{width: 220, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111", position: "absolute", zIndex: 0}}>
            </div>
            <div style = {{width: `${ this.state.internetbar }px`, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#5EC4EB", position: "absolute", zIndex: 1}}>
            </div>
            <div style = {{height: 10}}></div>
            {internetBox}
          </div>
        }
      </div>
    )
  }
}

export default WifiBox