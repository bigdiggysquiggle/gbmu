#!/usr/bin/env python3

import re
import requests
from bs4 import BeautifulSoup

soup = BeautifulSoup(requests.get('https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html').text, 'html.parser')
tab = soup.select('body > table:nth-child(4)')
tab = tab[0].find_all('tr')
endtab = []
for n in range(1, len(tab)):
	row = tab[n].find_all('td')
	for item in row[1:]:
		try:
			text = list(item.stripped_strings)
			endtab.append(text[1].split()[1])
		except:
			endtab.append('0')
proctab = []
for entry in endtab:
	string = entry.split("/")
	if string[0] == entry:
		proctab.append([ int(entry), 0 ])
	else:
		proctab.append([ int(string[0]), int(string[1]) ])
print("int test_cyctab[][2] = " + str(proctab).replace('[', '{').replace(']', '}') + ';')
