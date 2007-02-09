# Failed experiment: the __dict__ of MaskedArray is a dictproxy instance that
# is read-only
import MA, numpy
import rpy


_nan = float("NaN")

class RConvertibleMaskedArray(MA.MA.MaskedArray):

	def as_r(self):
		"""
		Convert to an R object
		"""
		#temp = numpy.array(self.filled(), dtype=float)

		# To compensate for unconvertible typecodes, various fill
		# values and whatnot, we just create a new array filling in
		# with NaNs as needed
		#res = numpy.fromiter(self._gen_floats(), dtype=float, count=numpy.prod(self.shape))
		len = numpy.prod(self.shape)
		oldmode = rpy.r.rep.local_mode()
		rpy.r.rep.local_mode(rpy.NO_CONVERSION)
		rpy.r.array.local_mode(rpy.NO_CONVERSION)
		rpy.r.aperm.local_mode(rpy.NO_CONVERSION)
		vec = rpy.r.rep(_nan, len)
		print "pre",; rpy.r.print_(vec)
		for i, x in enumerate(self.flat):
			print x
			if MA.getmask(x) != 1:
				vec[i] = float(x)
		print "post",; rpy.r.print_(vec)
		return rpy.r.aperm(rpy.r.array(data=vec, dim=[i for i in reversed(self.shape)]))

		#return numpy.fromfunction(lambda args: _to_float_array(self, args), self.shape)


def makeConvertible(obj):
	if isinstance(obj, MA.MA.MaskedArray):
		if hasattr(obj, 'as_r'):
			# Don't mutate objects that already have an as_r method
			pass
		else:
			obj.__class__ = RConvertibleMaskedArray

#setattr(obj, 'as_r', new.instancemethod(as_r, obj, obj.__class__))

#if 'as_r' in MA.MA.MaskedArray.__dict__:
#	print "Skipping duplicate"
#else:
#	MA.MA.MaskedArray.__dict__['as_r'] = 

