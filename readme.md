# AGPred
AGPred is an experimental C++ project for low-latency algorithmic trading for quote/trade streaming data providers (presently supports Polygon.io).  Trade and quote data is aggregated into interval bars, and a snapshot of the market state is available for use when algorithms are called. The snapshot includes the latest quotes/trades along with multiple intervals from 1min to 1week. 

Two example algorithms are included: `MA3EMA9Algo` (Moving Average 3/Exponential Moving Average 9) and `TFModelAlgo` (Tensorflow C++ model inference).


## Algorithms

Algorithms are defined by implementing the `AlgoBase2` class.

Entry and exit strategies are defined by implementing the `EntryBase` and `ExitBase` classes.

The algorithms are called with a snapshot of the market data (several time-steps of multiple intervals, fe. 1min, 5min, 1hr, etc.), the last 15,000 trades and the last 15,000 quotes, as well as processed data for all intervals and time-steps.

An example using Tensorflow model inference is provided in the example `TFModelAlgo` class/algorithm.

### Entries

Virtual class `EntryBase` is implemented to define an entry strategy.

The class is instantiated with a `frequency`, an `algo` and an `algo_pos`, which determine when the entry occurs.

When an algorithm returns a positive signal, an entry's virtual method `EntryData operator() (const Symbol& symbol, const Snapshot& snapshot) const` is called which returns an instance of `EntryData` that includes information such as the position type, size, stop-loss price, and limit price (0 for MARKET orders).

> Note: entries may define a non-zero stop-loss, and the system handles this stop-loss automatically. You may also create an exit strategy to handle stop-loss exits.

### Exits

Virtual classes `AlgoExitBase` or `SnapshotExitBase` are implemented to define an exit strategy.

In both cases, virtual method `ExitData operator() (const Position& position, const Symbol& symbol, const Snapshot& snapshot) const` is implemented to return an instance of `ExitData` that includes information such as the limit price (0 for MARKET orders). This method is called when an algorithm or a SnapshotExitBase call returns true.

#### AlgoExitBase

Instantiated with an `algo` and an `algo_pos` that ultimately determines when the exit occurs.

#### SnapshotExitBase

Virtual method `bool call(const Position& position, const Snapshot& snapshot) const` is called to determine if the exit should occur.


## Platforms

Different platforms are supported by implementing the `AccountAdapter` class.

A `Simulator` implementation of `AccountAdapter` is included, and this is used for both real-time testing and back testing algorithms. More information is included in the *Back-testing* section.


## Data

The project streams real-time quote/trade data.  However, back-testing and bulk downloads also use aggregated bar data in-place of downloading raw trades.

Presently only Polygon.io is implemented (directly, an adapter coding pattern for data providers has yet to be established).  

## Processed Data

### Technical Analysis

24 technical analysis signals including standard deviation, linear regression, OBV, ADI, PPO, ADX, and RSI.

Most signals are generated using TA-Lib.

### Candles

65 discrete/binary flags from candle analysis (fe. doji, hammer, evening-star, etc.).  


## Back-testing
`main_backtest.cpp` downloads historical data from Polygon.io, and runs instances of `AlgoBase2` on some interval.

After back-testing is complete, the following report is printed:
```
Back-Test complete.
  Profit Loss: $270
  Number of Trades: 10
  Commissions: $20
  Account Balance: $40,250
```

### Caching
The project presently does not cache data, however, a non-standard Squid proxy has been used to cache the results of API requests.


## Downloading

`main_dl.cpp` downloads data from Polygon.io, and saves it into numpy format for training ML models using the AGen python project.


## Utilities

`main_pred.cpp` performs inference against evaluation data used during training (with the python AGen project).  This allows verification of inference between the python and C++ project.  

`main_process.cpp` preprocesses aggregated trade data, for verification between python and C++ preprocessing routines.

`main_wss.cpp` streams quotes/trades from Polygon.io websockets, and performs real-time trading with the `Simulator` `AccountAdapter`.

