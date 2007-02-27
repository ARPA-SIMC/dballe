# Failed experiment: the __dict__ of MaskedArray is a dictproxy instance that
# is read-only
import volnd
import numpy
import numpy.core.ma as ma
import Numeric as numeric
import rpy

#dballe.volnd.Data.__dict__['as_r'] = new.instancemethod(fun, None, dballe.volnd.Data)

#def genfloat(a):
#	for x in a.flat:
#		if ma.getmask(x) != 1:
#			yield float(x)
#		else:
#			yield rpy.r.NAN

def ma_to_r(arr, dimnames=None):
	"""
	Convert a Masked Array to an R object
	"""

	vlen = numpy.prod(arr.shape)

	### This strategy fails as it gets converted as a recursive list and
	### not an array
	## To compensate for unconvertible typecodes, various fill
	## values and whatnot, we just create a new array filling in
	## with proper NaNs as needed
	#var = numpy.fromiter(genfloat(arr), dtype=float, count=len)
	#var.shape = arr.shape
	#rpy.r.print_(var)
	#return var

	# Trouble:
	#  - a numpy array is converted as a list of lists
	#  - I found no way to assign to an element in an R array
	#  - cannot use a tuple to index a numpy/ma array

	# Convert to floats
	farray = ma.array(arr, dtype=float)
	# Fill with NaNs
	farray = farray.filled(rpy.r.NAN)

	# Turn it into a Numeric array (bah, workaround the current python array mess)
	farray = numeric.array(farray)

	#old = numpy.seterr(invalid='ignore')
	#print farray 
	#numpy.seterr(**old)

	# Convert to an R array
	rpy.r.as_array.local_mode(rpy.NO_CONVERSION)
	farray = rpy.r.as_array(farray)

	# Add dimension names
	if dimnames:
		rpy.r.attr__.local_mode(rpy.NO_CONVERSION)
		farray = rpy.r.attr__(farray, "dimnames", dimnames)
		#rpy.r.print_(farray)

	return farray

	## Flatten shape to become a vector
	#farray = vlen

	#oldmode = rpy.r.rep.local_mode()
	#rpy.r.rep.local_mode(rpy.NO_CONVERSION)
	#rpy.r.array.local_mode(rpy.NO_CONVERSION)
	#rpy.r.aperm.local_mode(rpy.NO_CONVERSION)
	##vec = rpy.r.rep(rpy.r.NAN, vlen)
	##print "pre",; rpy.r.print_(vec)
	##print "ma2r to float"
	##for i, x in enumerate(farray.flat):
	##	if ma.getmask(x) != 1:
	##		vec[i] = float(x)
	##print "post",; rpy.r.print_(vec)
	##return rpy.r.array(data=vec, dim=[i for i in arr.shape])
	##print "ma2r to R"
	#if dimnames:
	#	return rpy.r.aperm(rpy.r.array(data=farray, dim=[i for i in reversed(arr.shape)], dimnames=[i for i in reversed(dimnames)]), perm=[i for i in reversed(range(1,len(arr.shape)+1))])
	#else:
	#	return rpy.r.aperm(rpy.r.array(data=farray, dim=[i for i in reversed(arr.shape)]), perm=[i for i in reversed(range(1,len(arr.shape)+1))])

def volnd_data_to_r(data):
	"""
	Convert a volnd data object to an R object
	"""
	dn = []
	for i in data.dims:
		dn.append(map(str, i))
	return ma_to_r(data.vals, dimnames=dn)

def volnd_save_to_r(vars, file):
	"""
	Convert the result of a volnd read into various R objects, and save them to the given file
	"""
	tosave = []
	for k, d in vars.iteritems():
		#print "s2r", k
		rpy.r.assign(k, volnd_data_to_r(d))
		tosave.append(k)
		for aname, adata in d.attrs.iteritems():
			rpy.r.assign(k+"."+aname, volnd_data_to_r(adata))
			tosave.append(k+"."+aname)
	rpy.r.save(list=tosave, file=file)
	# Cleanup the names from the environment
	rpy.r.remove(list=tosave)
