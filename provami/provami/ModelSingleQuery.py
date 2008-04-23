import Dballe

class Model:
	def __init__(self, dsn, user, password):
		# Init DB-ALLe and connect to the database
		self.lib = Dballe.LibInit()
		self.db = Dballe.DB(dsn, user, password)

		self.truncateResults = True
		self.lowerTruncateThreshold = 250
		self.resultsMax = 500
		self.resultsCount = 0
		self.resultsTruncated = False

		self.filter = Dballe.Record()

		self.updateListeners = []
		self.update()

	def registerUpdateListener(self, func):
		self.updateListeners.append(func)

	def update(self):
		self.results = []
		self.cache_stations = {}
		self.cache_levels = {}
		self.cache_tranges = {}
		self.cache_vtypes = {}
		self.cache_idents = {}
		self.cache_reports = {}
		self.filter.setc("query", "nosort,stream")
		dcur = self.db.query(self.filter)
		count = 0
		for record in dcur:
			self.results.append(record.copy())
			self.cache_stations[record.enqi("ana_id")] = [ \
			        	record.enqd("lat"), \
			        	record.enqd("lon"), \
			        	record.enqd("ident") ]
			self.cache_levels[",".join(map(lambda x: record.enqc(x), ["leveltype1", "l1", "leveltype2", "l2"]))] = 1
			self.cache_tranges[",".join(map(lambda x: record.enqc(x), ["pindicator", "p1", "p2"]))] = 1
			self.cache_vtypes[dcur.varcode()] = 1
			self.cache_idents[record.enqc("ident")] = 1
			self.cache_reports[record.enqi("rep_cod")] = record.enqc("rep_memo")
			count = count + 1
			if count % 10000 == 0:
				print count

		for l in self.updateListeners:
			l()

	# Fill up the ana list with all the ana stations
	def stations(self):
		for (k, v) in self.cache_stations.items():
			yield k, tuple(v)

	# Fill up the data list with all the data from the given ana
	def data(self):
		self.resultsCount = 0
		self.resultsTruncated = False
		for result in self.results:
			yield result
			self.resultsCount = self.resultsCount + 1
			# Bound to the real upper bound.
			if self.truncateResults and self.resultsCount > self.resultsMax:
				self.resultsCount = self.resultsCount - 1
				self.resultsTruncated = True
				break

	def attributes(self):
		result = Dballe.Record()
		self.db.attrQuery(self.currentContext, self.currentVarcode, result)
		for var in result:
			yield var

	def levels(self):
		for results in self.cache_levels.keys():
			yield results.split(',')

	def timeranges(self):
		for results in self.cache_tranges.keys():
			yield results.split(',')

	def variableTypes(self):
		for res in self.cache_vtypes.keys():
			yield res

	def idents(self):
		for res in self.cache_idents.keys():
			yield res

	def reports(self):
		for results in self.cache_reports.items():
			yield results

	def setiFilter(self, name, value):
		# If the value has changed, perform the update
		if value != self.filter.enqi(name):
			self.filter.seti(name, value)
			self.update()

	def setdFilter(self, name, value):
		# If the value has changed, perform the update
		if value != self.filter.enqd(name):
			self.filter.setd(name, value)
			self.update()

	def setcFilter(self, name, value):
		# If the value has changed, perform the update
		if value != self.filter.enqc(name):
			self.filter.setc(name, value)
			self.update()

	def setAreaFilter(self, latmin, latmax, lonmin, lonmax):
		updated = False
		if latmin != self.filter.enqd("latmin"):
			self.filter.setd("latmin", latmin)
			updated = True
		if latmin != self.filter.enqd("latmax"):
			self.filter.setd("latmax", latmax)
			updated = True
		if latmin != self.filter.enqd("lonmin"):
			self.filter.setd("lonmin", lonmin)
			updated = True
		if latmin != self.filter.enqd("lonmax"):
			self.filter.setd("lonmax", lonmax)
			updated = True
		if updated:
			self.update()

	def setLevelFilter(self, ltype1, l1, ltype2, l2):
		# If the value has changed, perform the update
		updated = False
		if ltype1 != self.filter.enqi("leveltype1"):
			self.filter.seti("leveltype1", ltype1)
			updated = True
		if l1 != self.filter.enqi("l1"):
			self.filter.seti("l1", l1)
			updated = True
		if ltype2 != self.filter.enqi("leveltype2"):
			self.filter.seti("leveltype2", ltype2)
			updated = True
		if l2 != self.filter.enqi("l2"):
			self.filter.seti("l2", l2)
			updated = True
		if updated:
			self.update()

	def setTimeRangeFilter(self, pind, p1, p2):
		# If the value has changed, perform the update
		updated = False
		if pind != self.filter.enqi("pindicator"):
			self.filter.seti("pindicator", pind)
			updated = True
		if p1 != self.filter.enqi("p1"):
			self.filter.seti("p1", p1)
			updated = True
		if p2 != self.filter.enqi("p2"):
			self.filter.seti("p2", p2)
			updated = True
		if updated:
			self.update()

	def setIdentFilter(self, mobile, ident):
		updated = False
		if mobile != self.filter.enqc("mobile"):
			self.filter.setc("mobile", mobile)
			updated = True
		if ident != self.filter.enqc("ident"):
			self.filter.setc("ident", ident)
			updated = True
		if updated:
			self.update()

	def setResultLimit(self, limit):
		updated = False
		if limit == None and self.truncateResults:
			self.truncateResults = False;
			updated = True;
		else:
			if not self.truncateResults:
				self.truncateResults = True
				updated = True
			if self.resultsMax != limit:
				self.resultsMax = limit
				updated = True
		if updated:
			self.update()
