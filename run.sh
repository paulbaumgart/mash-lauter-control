#!/bin/bash

python interface/beer-control.py $@ "log/$(date | tr ' ' _).csv"
