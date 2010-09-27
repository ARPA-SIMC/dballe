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
			self.cache_stations[record["ana_id"]] = [ \
			        	record["lat"], \
			        	record["lon"], \
			        	record["ident"] ]
			self.cache_levels[",".join(map(lambda x: record.enqc(x), ["leveltype1", "l1", "leveltype2", "l2"]))] = 1
			self.cache_tranges[",".join(map(lambda x: record.enqc(x), ["pindicator", "p1", "p2"]))] = 1
			self.cache_vtypes[dcur.varcode()] = 1
			self.cache_idents[record["ident"]] = 1
			self.cache_reports[record["rep_cod"]] = record["rep_memo"]
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
		if latmin != self.filter.get("latmin", None):
			self.filter["latmin"] = latmin
			updated = True
		if latmin != self.filter.get("latmax", None):
			self.filter["latmax"] = latmax
			updated = True
		if latmin != self.filter.get("lonmin", None):
			self.filter["lonmin"] = lonmin
			updated = True
		if latmin != self.filter.get("lonmax", None):
			self.filter["lonmax"] = lonmax
			updated = True
		if updated:
			self.update()

	def setLevelFilter(self, ltype1, l1, ltype2, l2):
		# If the value has changed, perform the update
		updated = False
		if ltype1 != self.filter.get("leveltype1", None):
			self.filter["leveltype1"] = ltype1
			updated = True
		if l1 != self.filter.get("l1", None):
			self.filter["l1"] = l1
			updated = True
		if ltype2 != self.filter.get("leveltype2", None):
			self.filter["leveltype2"] = ltype2
			updated = True
		if l2 != self.filter.get("l2", None):
			self.filter["l2"] = l2
			updated = True
		if updated:
			self.update()

	def setTimeRangeFilter(self, pind, p1, p2):
		# If the value has changed, perform the update
		updated = False
		if pind != self.filter.get("pindicator", None):
			self.filter["pindicator"] = pind
			updated = True
		if p1 != self.filter.get("p1", None):
			self.filter["p1"] = p1
			updated = True
		if p2 != self.filter.get("p2", None):
			self.filter["p2"] = p2
			updated = True
		if updated:
			self.update()

	def setIdentFilter(self, mobile, ident):
		updated = False
		if mobile != self.filter.get("mobile", None):
			self.filter["mobile"] = mobile
			updated = True
		if ident != self.filter.get("ident", None):
			self.filter["ident"] = ident
			updated = True
		if updated:
			self.update()

	def setResultLimit(self, limit):
		updated = False
		if limit is None and self.truncateResults:
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
