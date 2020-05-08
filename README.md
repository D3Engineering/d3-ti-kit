# Basic Setup

Technical documentation resides in the docs directory. This provides minimal
setup, and instructions for building the main docs.

## Python Environment

To setup the python environment run the following

    python3 -m venv .venv
    . .venv/bin/activate
    pip3 install -r requirements.txt

## Building the docs

To build the docs follow steps below. The entry point will be docs/\_build/index.html

    cd docs
    make html
