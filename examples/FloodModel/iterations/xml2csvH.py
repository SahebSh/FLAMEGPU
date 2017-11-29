#!/usr/bin/python
#
# Author: Mozhgan Kabiri Chimeh
#
# Any questions, please contact m.kabiri-chimeh@sheffiled.ac.uk
#
# Date : November 2017
#
# Description:
# Converts the FLAMEGPU output xml file to a csv file to be used for visualization with Paraview
#
# Usage: 
# python3 xml2csv.py -i 0.xml -o 0.csv
#---------------------------------------------------------------------

import getopt, sys
import xml.etree.ElementTree as ET

inputFile = ""
outputFile = ""

####################

try:
   # opts, args = getopt.getopt(sys.argv[1:], "i:o:", ["in=", "out="])
    opts, args = getopt.getopt(sys.argv[1:], "hi:o:", ["in=", "out="])
except getopt.GetoptError:
    # print help information and exit:
    print ('xml2csv.py -i <inputModel> -o <output>')
    sys.exit(2)
    
for o, a in opts:
    if o == '-h':
        print ('xml2csv.py -i <inputModel> -o <output>')
        sys.exit()
    elif o in ("-i", "--in"):
        inputFile = str(a)
    elif o in ("-o", "--out"):
        outputFile = str(a)
        
print ("Input file is %s"  % inputFile)
print ("Output file is %s" % outputFile)
        
####################

# Open XML document using parser
#tree = ET.parse('0.xml')
tree = ET.ElementTree(file=inputFile)
root = tree.getroot()

file = open(outputFile, "w") # opens a file

####################
   
parent = root.findall('xagent') # excludes other tags such as 'environment'

for child in parent:
    for node in child:
       # if node.tag != 'name':   # excludes the name of the agent
      if node.tag == 'h' :
           file.write("%s" % node.text)
    file.write("\n")
    
file.close()
