#!/bin/bash

cd /afs/cern.ch/user/d/dmbelyts/CMSSW_15_0_6/src/Demo/analyzer_demo
cmsenv

input_file="inFile=$1"
output_file="outFile=$2"
echo "Starting cmsRun with input file $input_file"

cmsRun python/demo_cfg.py $input_file $output_file
