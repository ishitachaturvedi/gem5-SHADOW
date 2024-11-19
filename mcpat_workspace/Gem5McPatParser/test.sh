#/bin/bash
# Test this on sample
#

# command to run script
CMD="python3 Gem5McPATParser.py"

# config file
SAMPLE_CONFIG="sample_m5out/config.json"

# stats txt file
SAMPLE_STATS="sample_m5out/stats.txt"

# template file x86
TEMPLATE_X86="templates/template_x86.xml"

# template arm
TEMPLATE_ARM="templates/template_arm.xml"

# outfile
OUTFILE="mcpat-in.xml"


rm $OUTFILE 2> /dev/null

$CMD -c $SAMPLE_CONFIG -s $SAMPLE_STATS -t $TEMPLATE_X86 >> /dev/null

if [[ $? -eq 0 ]]
then
   echo "X86 Pass"
else
    echo "X86 Fail"
fi



rm $OUTFILE 2> /dev/null

$CMD -c $SAMPLE_CONFIG -s $SAMPLE_STATS -t $TEMPLATE_ARM >> /dev/null

if [[ $? -eq 0 ]]
then
   echo "ARM Pass"
else
    echo "ARM Fail"
fi





