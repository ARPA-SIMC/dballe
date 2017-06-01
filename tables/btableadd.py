#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import math

# una riga di dballe.txt
class DballeTableEntry:
    def __init__(self, line=""):
        if line != "":
            self.bcode=line[1:7]
            self.name = line[8:72].rstrip()
            self.unitb = line[73:97].rstrip()
            self.scaleb = int(line[98:101])
            self.refb = int(line[102:114])
            self.widthb = int(line[115:118])
            self.unitc = line[119:142].rstrip()
            self.scalec = int(line[143:146])
            self.widthc = int(line[147:157])


    def Output(self, fileobj=sys.stdout):
        fileobj.write(" %s %-64s %-24s %3s %12s %3s %-23s %3s %10s\n" % 
                      (self.bcode, self.name, self.unitb,
                       self.scaleb, self.refb, self.widthb,
                       self.unitc, self.scalec, self.widthc))


# dballe.txt
class DballeTable:
    def __init__(self, filename="/usr/share/wreport/dballe.txt"):
        #TODO: use the local dballe.txt instead of the system one
        self.table = []
        for line in open(filename).readlines():
            self.table.append(DballeTableEntry(line))


    def Output(self, fileobj=sys.stdout):
        for entry in self.table:
            entry.Output(fileobj)


    def AddEntry(self, bcode, name, unit, scale, ref, widthb, widthc):
        nentry = 0
        for nentry in range(len(self.table)):
            if self.table[nentry].bcode > bcode: break
        else:
            nentry = nentry + 1
        self.table.insert(nentry, DballeTableEntry())

        self.table[nentry].bcode = bcode
        self.table[nentry].name = name
        self.table[nentry].unitb = unit
        self.table[nentry].scaleb = scale
        self.table[nentry].refb = ref
        self.table[nentry].widthb = widthb
        self.table[nentry].unitc = unit
        self.table[nentry].scalec = scale
        self.table[nentry].widthc = widthc


# categorie WMO per bufr
wmobufrcat = (
    {"x": "01", "name": "Identification", "expl": "Identifies origin and type of data"},
    {"x": "02", "name": "Instrumentation", "expl": "Defines instrument types used"},
    {"x": "03", "name": "Reserved"},
    {"x": "04", "name": "Location (time)", "expl": "Defines time and time derivatives"},
    {"x": "05", "name": "Location (horizontal - 1)", "expl": "Defines geographical position, including horizontal derivatives, in association with Class 06 (first dimension of horizontal space)"},
    {"x": "06", "name": "Location (horizontal - 2)", "expl": "Defines geographical position, including horizontal derivatives, in association with Class 05 (second dimension of horizontal space)"},
    {"x": "07", "name": "Location (vertical)", "expl": "Defines height, altitude, pressure level, including vertical derivatives of position"},
    {"x": "08", "name": "Significance qualifiers", "expl": "Defines special character of data"},
    {"x": "09", "name": "Reserved"},
    {"x": "10", "name": "Non-coordinate location (vertical)", "expl": "Height, altitude, pressure and derivatives observed or measured, not defined as a vertical location"},
    {"x": "11", "name": "Wind and turbulence", "expl": "Wind speed, direction, etc."},
    {"x": "12", "name": "Temperature"},
    {"x": "13", "name": "Hydrographic and hydrological elements", "expl": "Humidity, rainfall, snowfall, etc."},
    {"x": "14", "name": "Radiation and radiance"},
    {"x": "15", "name": "Physical/chemical constituents"},
    {"x": "19", "name": "Synoptic features"},
    {"x": "20", "name": "Observed phenomena", "expl": "Defines present/past weather, special phenomena, etc."},
    {"x": "21", "name": "Radar data"},
    {"x": "22", "name": "Oceanographic elements"},
    {"x": "23", "name": "Dispersal and transport"},
    {"x": "24", "name": "Radiological elements"},
    {"x": "25", "name": "Processing information"},
    {"x": "26", "name": "Non-coordinate location (time)", "expl": "Defines time and time derivatives that are not coordinates"},
    {"x": "27", "name": "Non-coordinate location (horizontal - 1)", "expl": "Defines geographical positions, in conjunction with Class 28, that are not coordinates"},
    {"x": "28", "name": "Non-coordinate location (horizontal - 2)", "expl": "Defines geographical positions, in conjunction with Class 27, that are not coordinates"},
    {"x": "29", "name": "Map data"},
    {"x": "30", "name": "Image"},
    {"x": "31", "name": "Data description operator qualifiers", "expl": "Elements used in conjunction with data description operators"},
    {"x": "33", "name": "Quality information"},
    {"x": "35", "name": "Data monitoring information"},
    {"x": "40", "name": "Satellite data"},
    {"x": "48", "name": "Reserved for local use"},
    {"x": "49", "name": "Reserved for local use"},
    {"x": "50", "name": "Reserved for local use"},
    {"x": "51", "name": "Reserved for local use"},
    {"x": "52", "name": "Reserved for local use"},
    {"x": "53", "name": "Reserved for local use"},
    {"x": "54", "name": "Reserved for local use"},
    {"x": "55", "name": "Reserved for local use"},
    {"x": "56", "name": "Reserved for local use"},
    {"x": "57", "name": "Reserved for local use"},
    {"x": "58", "name": "Reserved for local use"},
    {"x": "59", "name": "Reserved for local use"},
    {"x": "60", "name": "Reserved for local use"},
    {"x": "61", "name": "Reserved for local use"},
    {"x": "62", "name": "Reserved for local use"},
    {"x": "63", "name": "Reserved for local use"},
)

def wmobufrcatHelp():
    for entry in wmobufrcat:
        print entry["x"], entry["name"]
        if entry.has_key("expl"):
            print entry["expl"]


# importo dballe.txt
bufrtable = DballeTable()
outfile = "out.txt"
# chiedo la categoria
nocat = True
while nocat:
    print "Categoria (01-63, 00=help)?"
    cat = sys.stdin.readline()[:-1]
    for catdesc in wmobufrcat:
        if catdesc["x"] == cat:
            nocat = False
            break
    else:
        wmobufrcatHelp()

# cerco un codice B libero nella tabella locale
if int(cat) < 48:
    pstart = 192
else:
    pstart = 1
for param in range(pstart, 256):
    b = "0%s%03d" % (cat,param)
    if not any(t.bcode == b for t in bufrtable.table):
        break
else:
    print "mi dispiace ma non c'è posto"
    sys.exit(1)

# trovato un codice libero
print "Inserisco una nuova variabile con codice ",b
# chiedo il resto dell'informazione
print "nome ?"
name = sys.stdin.readline()[:-1]
print "unità di misura ?"
unit = sys.stdin.readline()[:-1]
print "minimo plausibile ?"
minp = float(sys.stdin.readline()[:-1])
print "massimo plausibile ?"
maxp = float(sys.stdin.readline()[:-1])
print "precisione (variazione mimina apprezzabile del dato)?"
prec = float(sys.stdin.readline()[:-1])
scale = int(-math.floor(math.log10(prec)))
ref = int(minp*math.pow(10., scale))
widthb = int(math.ceil(math.log((maxp-minp)/prec+2)/math.log(2.0)))
widthc = int(math.ceil(math.log10(max(abs(minp),abs(maxp))/prec+1)))

# aggiungo la variabile
bufrtable.AddEntry(b, name, unit, scale, ref, widthb, widthc)
# stampo la tabella
print "la nuova tabella è in",outfile
bufrtable.Output(open(outfile,"w"))

# scale[bufr] = -floor(LOG10(Precisione))
# refval[bufr] = Valoremin*10^scale[bufr]
# datawidth[bufr] = ceil(LOG2((Valoremax-Valoremin)/Precisione))
# scale[crex] = scale[bufr]
# datawidth[crex] = ceil(LOG10((Valoremax-Valoremin)/Precisione))
