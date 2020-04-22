import React, { Component } from 'react';
import * as geolib from 'geolib';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome'
import { faCircleNotch } from '@fortawesome/free-solid-svg-icons'

class CPUBox extends Component {
  constructor(props) {
    super(props)
    this.state = {cores: 0, corebar: 40}
  }

  MeasureCores = () => {
    const si = require('systeminformation')
    si.cpu().then(data => {
      this.setState({cores: data.cores})
      this.setState({corebar: Math.min(Math.max(40, 200 * (data.cores / 32)), 200)})
    })
  }

  componentDidMount() {
    this.MeasureCores()
  }

  render() {
    let cpuBox;

    if(this.state.cores < 4) {
      cpuBox =
      <div>
       <div style = {{background: "none", border: "solid 1px #d13628", height: 6, width: 6, borderRadius: 4, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
       </div>
        <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
          {this.state.cores} Core CPU
        </div>
        <div style = {{marginTop: 8, fontSize: 12, color: "#333333", paddingLeft: 20, lineHeight: 1.4}}>
          You need at least three logical CPU cores to run Fractal.
        </div>
      </div>
    } else {
      cpuBox =
      <div>
       <div style = {{background: "none", border: "solid 1px #14a329", height: 6, width: 6, borderRadius: 4, borderRadius: 4, display: "inline", float: "left", position: 'relative', top: 5, marginRight: 13}}>
       </div>
        <div style = {{marginTop: 5, fontSize: 14, fontWeight: "bold"}}>
          {this.state.cores} Core CPU
        </div>
        <div style = {{marginTop: 8, fontSize: 12, color: "#333333", paddingLeft: 20, lineHeight: 1.4}}>
          Your CPU has enough logical cores to run Fractal.
        </div>
      </div>
    }

    return (
      <div style = {{marginTop: 20}}>
        {
          this.state.cores === 0
          ?
          <div style = {{marginTop: 25, position: 'relative', right: 5}}>
            <FontAwesomeIcon icon = {faCircleNotch} spin style = {{color: "#5EC4EB", height: 10, display: 'inline', marginRight: 9}}/>
            <div style = {{marginTop: 8, fontSize: 12, color: "#333333", display: 'inline'}}>
              Scanning CPU
            </div>
          </div>
          :
          <div>
            <div style = {{height: 10}}></div>
            {cpuBox}
          </div>
        }
      </div>
    )
  }
}

export default CPUBox