# Quantik solver

Implementation of a strong solver for [Quantik](https://en.gigamic.com/game/quantik), a 2-player adversarial abstract strategy game by Nouri Khalifa and published by Gigamic.

**PROJECT STATUS:** Currently in development. Come back soon!

## Developer notes

### Requirements

The supported platform to build the solver and opening book is Linux.
For the solver, the given makefile uses `gcc` to build the solver. 
`gcc` is installed on most Linux distributions, together with the GNU C standard library.

To use Python bindings for Quantik (e.g. to create the opening book manually), one needs Python, Cython and setuptools (see `pyproject.toml`).

### Building the solver

The Quantik solver is implemented as a simple-to-use terminal program.
To build and run it, simply clone the repository and run `make quantik_solver` from the repository root. 
This creates the executable `quantik_solver`, which should be run together with `opening book
*Note that to be able to run the solver, the executable must reside in the same directory as* `opening_book.bin`.

### Installing the Python bindings

To independently create the opening book, we implemented Python bindings for the Quantik and minimax search implementations.
We recommend installing the bindings in a virtual environment, as described below.

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

### To do list

A minimal working implementation of the solver is now ready to go. However, there's a bunch of code cleanup and documentation to do.

- complete unimplemented functions of the solver
- make the code pretty to read
- write a short report on the solver implementation
- complete and finalize README on usage
- distribute the executable and opening book as a package
