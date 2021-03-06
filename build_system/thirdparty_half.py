# FEAT3: Finite Element Analysis Toolbox, Version 3
# Copyright (C) 2010 - 2020 by Stefan Turek & the FEAT group
# FEAT3 is released under the GNU General Public License version 3,
# see the file 'copyright.txt' in the top level directory for details.
__author__ = "Dirk Ribbrock"
__date__   = "Feb 2016"
from build_system.thirdparty_package import ThirdpartyPackage
import os

class HALF(ThirdpartyPackage):

  def __init__(self,trunk_dirname):
    self.names = ["half"]
    self.dirname = "half"
    self.filename = "half-2.1.0.zip"
    self.url = "http://downloads.sourceforge.net/project/half/half/2.1.0/half-2.1.0.zip?r=http%3A%2F%2Fhalf.sourceforge.net%2Findex.html&ts=1581415216&use_mirror=netcologne"
    self.cmake_flags = " -DFEAT_HAVE_HALFMATH:BOOL=ON"
    self.trunk_dirname = trunk_dirname
    self.target_dirname = trunk_dirname+os.sep+self.dirname
