#!/usr/bin/env python
# vim: set filetype=python sw=2 sts=2 et nofoldenable :
# This is a python script.
# If you encounter problemes when executing it on its own, start it with a python interpreter
import sys
import re
import array
# This is a very primitive script for pre processing Maple generated C code

# First argument is the name of this file, so strip that
args = sys.argv[1:]

if( ( (len(args) == 0) or len(args) > 2) or ("help" in " ".join(args)) or ("?" in " ".join(args)) ):
  print("Usage: preprocess_maple_output [source] [output file]")
  print("If [output_file] is not present, this script will write to preprocessed_[source]")
  sys.exit(1)


source_file = open(args[0],'r')

# Read output file or set default
if(len(args) == 2):
  output_file = open(args[1],'w')
else:
  output_file = open("preprocessed_"+args[0],'w')

# It would be nicer to compile all regular expressions before this, but the files are small anyway

# Iterate over the input file
for line in source_file:
  # Skip lines starting with the Maple prompt ">" or a warning
  if ( re.match('^>.*',line) or re.match('^Warning',line) ):
    continue

  output_line = line

  # Replaces grad[i][j] by grad(i,j)
  output_line = re.sub(r'grad\[(.)\]\[(.)\]', r'grad(\1,\2)', output_line)
  # Replaces grad_norm[i][j] by grad_norm(i,j)
  output_line = re.sub(r'grad_norm\[(.)\]\[(.)\]', r'grad_norm(\1,\2)', output_line)
  # Replaces x[i][j] by x(i,j)
  output_line = re.sub(r'x\[(.)\]\[(.)\]', r'x(\1,\2)', output_line)
  # Replaces h[i] by h(i)
  output_line = re.sub(r'h\[(.)\]', r'h(\1)', output_line)
  # Replace fac_reg by this->fac_reg
  output_line = re.sub(r'fac_', r'this->_fac_', output_line)
  # Encloses the value of each assignment by DataType_( ), i.e. "norm_ = value;" -> "norm_ = DataType_(value);
  #output_line = re.sub(r'(^.*=)(.*);', r'\1 DataType_(\2);', output_line)
  # Transforms all occurences "gradij" -> "grad(i,j)"
  #output_line = re.sub('(?<!_)grad(.)(.)',r"grad(\1,\2)",output_line)
  # Replace pow by FEAST2's Math::pow
  output_line = re.sub('pow',"Math::pow",output_line)
  # Replace sqrt by FEAST2's Math::sqrt
  output_line = re.sub('sqrt',"Math::sqrt",output_line)
  # Cast all Maple generated doubles to DataType_. In case of quad precision, this might lose significant bits
  # TODO: Figure out how to change the Maple output to use integers that can be casted to DataType_
  output_line = re.sub(r'(0\.[1-9]*e[1-9])',r'DataType(\1)',output_line)
  output_line = re.sub(r'0\.([1-9])e1',r'\1',output_line)
  output_line = re.sub(r'0\.([1-9][0-9])e2',r'\1',output_line)
  output_line = re.sub(r'0\.([1-9][0-9][0-9])e3',r'\1',output_line)

  output_file.write(output_line)

source_file.close()
output_file.close()
print("Preprocessed file " + args[0] + " and wrote output to " + str(output_file))

sys.exit(0)