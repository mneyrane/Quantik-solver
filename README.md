# Quantik solver

Implementation of a solver for [Quantik](https://en.gigamic.com/game/quantik), a 2-player adversarial abstract strategy game by Nouri Khalifa and published by Gigamic.
To download the solver, check out the [releases page](https://github.com/mneyrane/Quantik-solver/releases).
The solver is interacted with a simple-to-use command-line interface, and is currently supported to run on Linux.
The usage help and instructions are provided in the program itself by entering the command `h`.


## Clarifications


### What exactly is meant by "solver"?

Our solver *strongly solves* Quantik, that is, it computes a winning strategy (if the current player to move has one) starting from any legal board position, under practical time and space constraints.
In our case, from any board position, determining actions that force a win takes less than a second on a standard desktop computer!

### How does the solver carry out its evaluation?

The heart of the solver consists of two ideas.
The first is a pruned minimax algorithm to efficiently evaluate winning and losing moves.
The second is a precomputation of such an evaluation for all early game states and caching the results into an opening book.
This on its own sounds expensive, but one can exploit symmetry (board rotations, board reflections, special column and row swaps, permuting piece shapes) to limit the search to a subset of states.
The same symmetries can be used to make efficient storage and querying of said states.
Unfortunately, due to my preoccupation with other activities, a technical writeup of the aforementioned ideas have been postponed indefinitely.


## Developer notes


### Requirements

The supported platform to build the solver and opening book is Linux.
For the solver, the given makefile uses `gcc` to build the solver. 
`gcc` is installed on most Linux distributions, together with the GNU C standard library.

To use Python bindings for Quantik (e.g. to create the opening book manually), one needs Python, Cython and setuptools (see `pyproject.toml`).

### Building the solver

To build and run the Quantik solver, simply clone the repository and run `make quantik_solver` from the repository root. 
This creates the executable `quantik_solver`, which should be run together with `opening book
*Note that to be able to run the solver, the executable must reside in the same directory as* `opening_book.bin`.

### Building the opening book

To independently create the opening book, we implemented Python 3 bindings for the Quantik and minimax search implementations.
We recommend installing the bindings in a virtual environment, as described below.

Building the bindings requires `setuptools`, `Cython` and `wheel`.
To run the opening book creation itself additionally requires `sympy` and `networkx`.
These are listed as dependencies in `pyproject.toml` and will be installed within the virtual environment.

In the repository root, create the virtual environment like so:
```
$ mkdir virtualenv
$ python3 -m venv virtualenv
```
Then source the environment for the current shell and install the bindings via `pip`:
```
$ source virtualenv/bin/activate
(virtualenv) $ pip install -e .
```
Now the opening book can be built by running the scripts in `opening_book/`:
```
(virtualenv) $ cd opening_book
(virtualenv) $ python3 01-create_opening_book.py
...
(virtualenv) $ python3 02-binarize_data.py
...
```
