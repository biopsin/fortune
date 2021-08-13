# fortune
Fortune is a small game that is meant to lighten up your life.
It can be used to display a random entry from a cookie file.

So why another fortune program?  Because the BSD one sucks.  It needs a
separate file and a program called "strfile" to create that said file.
Modern systems have mmap() which makes a simple fortune program almost
trivial to write, so I did it.

Felix von Leitner <felix-fortune@fefe.de>

# note
Personal backup of source files in case http://dl.fefe.de disappear.
