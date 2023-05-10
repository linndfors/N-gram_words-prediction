import React, { Component } from 'react';
import './App.css';


class App extends Component {
  constructor(props) {
    super(props);
    this.state = {
      n: 0,
      numToPredict: 0,
      path: '',
      context: '',

      resultTrain: {},
      resultPredict: {},
      resultPrevious: {}
    }

    this.apiUrl = 'http://localhost:8080';

    this.handleNInput = this.handleNInput.bind(this);
    this.handlePathInput = this.handlePathInput.bind(this);
    this.handleNumToPredictInput = this.handleNumToPredictInput.bind(this);
    this.handleContextInput = this.handleContextInput.bind(this);

    this.handleTrainSubmit = this.handleTrainSubmit.bind(this);
    this.handlePredictSubmit = this.handlePredictSubmit.bind(this);
  }

  handleNInput(event) {
    this.setState({n: event.target.value});
  }

  handlePathInput(event) {
    this.setState({path: event.target.value});
  }

  handleNumToPredictInput(event) {
    this.setState({numToPredict: event.target.value});
  }

  handleContextInput(event) {
    this.setState({context: event.target.value});
  }

    async handleTrainSubmit(event) {
        event.preventDefault();
        const url = `${this.apiUrl}/train?path=${this.state.path}&n=${this.state.n}`;
        fetch(url) //, {headers: new Headers({
            // "ngrok-skip-browser-warning": "69420"})} )
            .then(res => res.json())
            .then(res => this.setState({ path: this.state.path,
                n: this.state.n,
                resultTrain: res }));
    }

    async handlePredictSubmit(event) {
        event.preventDefault();
        const url = `${this.apiUrl}/predict?n=${this.state.n}&num_to_predict=${this.state.numToPredict}&context=${this.state.context}`;
        fetch(url) //, {headers: new Headers({
            // "ngrok-skip-browser-warning": "69420"})} )
            .then(res => res.json())
            .then(res => this.setState({ n: this.state.n,
                numToPredict: this.state.numToPredict,
                context: this.state.context,
                resultPredict: res }));
    }

  render() {
    return (
        <div>
          <form onSubmit={this.handleTrainSubmit}>
            <label>
              Enter n:
              <input type="number" value={this.state.n} onChange={this.handleNInput} />
            </label>
            <label>
              Enter directory of Gutenberg files:
              <input type="text" value={this.state.path} onChange={this.handlePathInput} />
            </label>
            <button type="submit">Train</button>
          </form>
            {this.state.resultTrain.output ?
                <p>Result: {this.state.resultTrain.output.map((item, i) => <p key={i}>{item}</p>)}</p>
                : <p>Result: </p>}


          <form onSubmit={this.handlePredictSubmit}>
            <label>
              Enter n:
              <input type="number" value={this.state.n} onChange={this.handleNInput} />
            </label>
            <label>
              Enter number of words to predict:
              <input type="number" value={this.state.numToPredict} onChange={this.handleNumToPredictInput} />
            </label>
            <label>
              Enter context:
              <input type="text" value={this.state.context} onChange={this.handleContextInput} />
            </label>
            <button type="submit">Predict</button>
          </form>
            {this.state.resultPredict.output ?
                <p>Result: {this.state.resultPredict.output.map((item, i) => <p key={i}>{item}</p>)}</p>
                : <p>Result: </p>}
        </div>
    );
  }
}

export default App;
