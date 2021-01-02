#!/usr/bin/python3

import sys
import sqlite3

sqldbf= '/home/giacomo/.config/cumbia/cumbia-build/cumbia-build.db'

class Indexes:
	ENV=0
	PATH=1
	APP=2
	ARGS=3
	DATETIME=4

class App:
	def __init__(self, cmdli, datet):
		self.cmdline = cmdli
		self.datetime = datet
	

def add(cmd, args, env, tbl):
	conn = sqlite3.connect(sqldbf)
	c = conn.cursor()
	# path in cmd?
	n = cmd.count('/') 
	path = ''
	if n > 0:
		sp = cmd.split('/')
		path = '/'.join(sp[0:len(sp)-1])
		appnam = sp[len(sp) - 1]
	else:
		appnam = cmd
	t = (appnam,path)
	c.execute("SELECT rowid FROM apps WHERE name=? AND path=?", t)
	res = c.fetchall()
	if len(res) > 0:
		print("found " + appnam)
		rowid = res[0][0]
	else:
		c.execute("INSERT INTO apps (name,path) VALUES (?,?)", t)
		rowid = c.lastrowid

	if tbl == 'history' or tbl == 'test':
		t = (rowid, env, args)
		c.execute("REPLACE INTO " + tbl + " (id,env,args,datetime) VALUES (?,?,?,datetime())", t)
	
	conn.commit()
	conn.close()

def find(appnam, tbl):
	conn = sqlite3.connect(sqldbf)
	c = conn.cursor()
	t = (appnam,)
	
	fields = "%s.env,%s.path,%s.name,%s.args,%s.datetime" % (tbl,"apps","apps",tbl,tbl)
	stmt = "SELECT " + fields + " FROM apps," + tbl + " WHERE apps._rowid_=" + tbl + ".id AND apps.name=?"
	c.execute(stmt, t)
	res = c.fetchall()

	return res

def mkcmdlines(cmdls):
	cmdlines = []
	for args in cmdls:
		if len(args) > 3 and len(args[1]) == 0: # no path
			cmdlines.append(App( '%s %s %s' % (args[Indexes.ENV], args[Indexes.APP], args[Indexes.ARGS]), args[Indexes.DATETIME]))
		elif len(args) > 3:
			cmdlines.append(App( '%s %s/%s %s' % (args[Indexes.ENV], args[Indexes.PATH], args[Indexes.APP], args[Indexes.ARGS]), args[Indexes.DATETIME]))
	return cmdlines

def main():
	asiz = len(sys.argv)
	a = []
	if asiz == 5:
		# curun-db.py add path/to/app VARIABLE1=val1 arg1 arg2 ... argN
		tbl = ''
		if sys.argv[1] == "add":
			tbl = 'history'
		elif sys.argv[1] == "addtest":
			tbl = 'test'
		if len(tbl) > 0:
			add(sys.argv[2], sys.argv[3], sys.argv[4], tbl)

	elif asiz == 3 and sys.argv[1] == "find":
		a = find(sys.argv[2], "history")
	elif asiz == 3 and sys.argv[1] == "findtest":
		a = find(sys.argv[2], "test")


	cmdlines = mkcmdlines(a)
	for c in cmdlines:
		print("app: %s [%s]" % (c.cmdline,c.datetime))
	

if __name__ == "__main__":
    main()

