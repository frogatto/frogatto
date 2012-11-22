from __future__ import print_function
import urllib
from HTMLParser import HTMLParser
stats_upload_url = "http://www.wesnoth.org/files/dave/frogatto-stats-1.1.1/"

class FrogattoStatsFilenames(HTMLParser):
	stats_files = []
	
	def __init__(self):
		HTMLParser.__init__(self)
		self.feed(urllib.urlopen(stats_upload_url).read())
		
	def handle_starttag(self, tag, attrs):
		if(attrs and attrs[0][0] == 'href') and attrs[0][1].endswith('.cfg'):
			#print("tag/attrs: ", tag, attrs)
			self.stats_files.append(attrs[0][1])
			
print("Downloading filenames...")
parse = FrogattoStatsFilenames()
print("Saving {0} filenames...".format(len(parse.stats_files)))
name_file = open("list of frogatto stats files.txt", 'w', 0)
print(parse.stats_files, file=name_file)
print("Done.")