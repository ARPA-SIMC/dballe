#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import os
import itertools
if os.environ.get("DBALLE_BUILDING_DOCS", "") != 'true':
    import numpy
    import numpy.ma as ma
    #import Numeric as numeric
    import rpy2.robjects as robjects
    import rpy2.rinterface as rinterface

# Failed experiment: the __dict__ of MaskedArray is a dictproxy instance that
# is read-only

#dballe.volnd.Data.__dict__['as_r'] = new.instancemethod(fun, None, dballe.volnd.Data)

#def genfloat(a):
#   for x in a.flat:
#       if ma.getmask(x) != 1:
#           yield float(x)
#       else:
#           yield robjects.r.NAN

def ma_to_rlist(arr):
    """
    Create a list of floats or rinterface.NA_Real from a masked array, raveled
    in R value order
    """
    ma_data = arr.data.ravel("F")
    ma_mask = arr.mask.ravel("F")
    rlist = []
    for val, mask in zip(ma_data, ma_mask):
        if mask:
            rlist.append(rinterface.NA_Real)
        else:
            rlist.append(val)
    return rlist

def ma_to_r(arr, dimnames=None):
    """
    Convert a Masked Array to an R object
    """
    # We copy data around way more than we probably should, but I found no way
    # to efficiently generate R NA values from a the MaskedArray mask

    # Convert to floats
    farray = ma.array(arr, dtype=float)

    # Convert to an R array, with NAs
    vec = rinterface.FloatSexpVector(ma_to_rlist(farray))
    dim = rinterface.IntSexpVector(farray.shape)
    if dimnames:
        # Create with dimension names, too
        dimnames = robjects.r.array(dimnames)
        farray = robjects.r.array(vec, dim=dim, dimnames=dimnames)
    else:
        farray = robjects.r.array(vec, dim=dim)

    return farray

def volnd_data_to_r(data):
    """
    Convert a volnd data object to an R object
    """
    rinterface.initr()

    dn = []
    for i in data.dims:
        dn.append([str(x) for x in i])
    return ma_to_r(data.vals, dimnames=dn)

def volnd_save_to_r(vars, file):
    """
    Convert the result of a volnd read into various R objects, and save them to the given file
    """
    rinterface.initr()

    tosave = []
    for k, d in vars.items():
        #print "s2r", k
        robjects.r.assign(k, volnd_data_to_r(d))
        tosave.append(k)
        for aname, adata in d.attrs.items():
            robjects.r.assign(k+"."+aname, volnd_data_to_r(adata))
            tosave.append(k+"."+aname)
    tosave = robjects.vectors.StrVector(tosave)
    robjects.r.save(list=tosave, file=file)
    # Cleanup the names from the environment
    robjects.r.remove(list=tosave)
