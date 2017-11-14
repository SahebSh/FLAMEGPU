#!/bin/bash

# script to convert all xml files in "iterations" folder to csv
# Make it executable first by running --> chmond +x xmlConv.sh
# To run the script --> ./xmlConv.sh

xmlFolder=iterations/*.xml

for fname in $xmlFolder
do
	echo "Processing $fname file ..."
	oname="${fname%.*}"
	python3 xml2csv.py -i $fname -o $oname.csv
done

echo "done converting xmls to csv"

