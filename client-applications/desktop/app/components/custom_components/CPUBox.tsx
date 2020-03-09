import React, { Component } from 'react';
import * as geolib from 'geolib';

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
      <div style = {{marginTop: 35}}>
        {
          this.state.cores === 0
          ?
          <div>
            <div style = {{width: 220, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111"}}>
            </div>
            <div style = {{marginTop: 8, fontSize: 11, color: "#D6D6D6"}}>
              <div style = {{background: "none", border: "solid 1px #5EC4EB", height: 6, width: 6, borderRadius: 3, display: "inline", float: "left", position: 'relative', top: 3, marginRight: 7}}>
              </div>
              Scanning CPU
            </div>
          </div>
          :
          <div>
            <div style = {{width: 220, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#111111", position: "absolute", zIndex: 0}}>
            </div>
            <div style = {{width: `${ this.state.corebar }px`, height: `${ this.props.barHeight }px`, borderRadius: "0px 2px 2px 0px", background: "#5EC4EB", position: 'absolute', zIndex: 1}}>
            </div>
            <div style = {{height: 10}}></div>
            {cpuBox}
          </div>
        }
      </div>
    )
  }
}

export default CPUBox