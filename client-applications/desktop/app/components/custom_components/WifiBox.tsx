import React, { Component } from 'react'
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'
import speedTest from 'speedtest-net'

class WifiBox extends Component {
  constructor(props) {
    super(props)
    this.state = {downloadSpeed: 0, uploadSpeed: 0, ping: 0}
    this.MeasureConnectionSpeed = this.MeasureConnectionSpeed.bind(this)
  }

  async MeasureConnectionSpeed() {
    try {
      let options = {
        acceptLicense: true,
        acceptGdpr: true
      }
      var speed = await speedTest(options);
      console.log(speed.download);
      this.setState({ downloadSpeed: (speed.download.bandwidth / 125000).toFixed(1) }); // bytes to megabits
      this.setState({ uploadSpeed: (speed.upload.bandwidth / 125000).toFixed(1) });
      this.setState({ ping: speed.ping.latency.toFixed(1) });
    } catch (err) {
      console.log(err);
    }
  }

  componentDidMount() {
    this.MeasureConnectionSpeed();
  }


  render() {
    let internetBox;
    
    if(this.state.downloadSpeed < 10) {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            {this.state.downloadSpeed} Mbps Internet
          </div>
          <div style = {{marginTop: 8, fontSize: 12, color: "#333333", lineHeight: 1.4, paddingLeft: 20}}>
            Your Internet bandwidth is slow. Try closing streaming apps like Youtube, Netflix, or Spotify.
          </div>
        </div>
    } else if(this.state.downloadSpeed < 20) {
      internetBox =
        <div>
          <div style = {{background: "none", border: "solid 1px #f2a20c", height: 6, width: 6, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
          </div>
          <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
            {this.state.downloadSpeed} Mbps Internet
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
            {this.state.downloadSpeed} Mbps Internet
          </div>
          <div style = {{marginTop: 8, fontSize: 12, color: "#333333", lineHeight: 1.4, paddingLeft: 20}}>
            Your Internet is fast enough to support high-quality streaming.
          </div>
        </div>
    }

    return (
      <div style = {{marginTop: 15}}>
        {
          this.state.downloadSpeed === 0
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