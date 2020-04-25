import React, { Component } from 'react'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'
import Style from '../../style/components/general.module.css'
import speedTest from 'speedtest-net'

class WifiBox extends Component {
  constructor(props) {
    super(props)
    this.MeasureConnectionSpeed = this.MeasureConnectionSpeed.bind(this);
    this.state = {
      downloadSpeed: 0,
      uploadSpeed: 0,
      ping: 0,
      internetbar: 50
    }
  }

  async MeasureConnectionSpeed() {
    try {
      var speed = await speedTest();
      this.setState({ downloadSpeed: (speed.download.bandwidth / 125000).toFixed(1) }); // bytes to megabits
      this.setState({ uploadSpeed: (speed.upload.bandwidth / 125000).toFixed(1) });
      this.setState({ ping: speed.ping.latency.toFixed(1) });
      //console.log(speed);
    } catch (err) {
      console.log(err);
    }

    // let component = this;
    // var imageAddr = "http://www.kenrockwell.com/contax/images/g2/examples/31120037-5mb.jpg";
    // var downloadSize = 4995374; //bytes
    // var startTime, endTime;
    // var download = new Image();
    // download.onload = function () {
    //   endTime = (new Date()).getTime();
    //   var duration = (endTime - startTime) / 1000;
    //   var bitsLoaded = downloadSize * 8;
    //   var speedBps = (bitsLoaded / duration).toFixed(2);
    //   var speedKbps = (speedBps / 1024).toFixed(2);
    //   var speedMbps = (speedKbps / 1024).toFixed(2);
    //   component.setState({ downloadSpeed: speedMbps });
    //   component.setState({ internetbar: Math.min(Math.max(speedMbps, 50), 200) })
    // }

    // startTime = (new Date()).getTime();
    // var cacheBuster = "?nnn=" + startTime;
    // download.src = imageAddr + cacheBuster;
  }

  componentDidMount() {
    this.MeasureConnectionSpeed();
  }

  render() {
    let internetBox;

    if (this.state.downloadSpeed < 10) {
      internetBox =
        <div>
          <div style={{ background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13 }} />
          <h1 className={Style.header + " mt-1"}>{this.state.downloadSpeed} Mbps Internet Down</h1>
          {/* <h1 className={Style.header + " pl-3 ml-1"}>{this.state.uploadSpeed} Mbps Internet Up</h1>
          <h1 className={Style.header + " pl-3 ml-1"}>{this.state.ping} ms Ping</h1> */}
          <p className={Style.text + " mt-3 pl-3 ml-1"}>
            Your Internet bandwidth is slow. Try closing streaming apps like Youtube, Netflix, or Spotify.
          </p>
        </div>
    } else if (this.state.downloadSpeed < 20) {
      internetBox =
        <div>
          <div style={{ background: "none", border: "solid 1px #f2a20c", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13 }} />
          <h1 className={Style.header + " mt-1"}>{this.state.downloadSpeed} Mbps Internet Down</h1>
          {/* <h1 className={Style.header + " pl-3 ml-1"}>{this.state.uploadSpeed} Mbps Internet Up</h1>
          <h1 className={Style.header + " pl-3 ml-1"}>{this.state.ping} ms Ping</h1> */}
          <p className={Style.text + " mt-3 pl-3 ml-1"}>
            Expect a smooth streaming experience with occassional image quality drops.
          </p>
        </div>
    } else {
      internetBox =
        <div>
          <div style={{ background: "none", border: "solid 1px #14a329", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13 }} />
          <h1 className={Style.header + " mt-1"}>{this.state.downloadSpeed} Mbps Internet Down</h1>
          {/* <h1 className={Style.header + " pl-3 ml-1"}>{this.state.uploadSpeed} Mbps Internet Up</h1>
          <h1 className={Style.header + " pl-3 ml-1"}>{this.state.ping} ms Ping</h1> */}
          <p className={Style.text + " mt-3 pl-3 ml-1"}>
            Your Internet is fast enough to support high-quality streaming.
          </p>
        </div>
    }

    return (
      <div className="mt-2">
        {
          this.state.downloadSpeed === 0
            ?
            <p className={Style.text + " mt-4 ml-1"} style={{ position: 'relative', right: 5 }}>
              <FontAwesomeIcon icon={faCircleNotch} spin style={{ color: "#5EC4EB", height: 10 }} />
              &nbsp;&nbsp;&nbsp;Checking Internet speed
            </p>
            :
            <div>
              {internetBox}
            </div>
        }
      </div>
    )
  }
}

export default WifiBox