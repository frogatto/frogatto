import string
def main(text_in):
	text_size = len(text_in)
	pos = 0 #start at the beginning of the text
	text_in += '\n    ' #bit of padding for the end.
	mode = 'normal'
	
	print (' == START OF TRANSLATION == \n\n')
	
	if text_in[pos:pos+5] == '#summ':
		while text_in[pos] != '\n':
			pos +=1
	
	while pos < text_size:
		
		if text_in[pos:pos+2] == '[[': #links
			pos += 2
			start = pos
			while text_in[pos:pos+2] != ']]':
				pos +=1
			parts = (text_in[start:pos].split('|'))
			if len(parts) == 1:
				print('[[' + parts[0] + ']]', end='')
			else:
				print('[' + parts[0] + '](' + parts[1] + ')', end='')
			pos += 2
			
		elif text_in[pos:pos+5] == '\n{{{\n': #code blocks
			pos += 4
			while text_in[pos:pos+4] != '\n}}}':
				if text_in[pos] == '\n':
					pos +=1
					print('\n    ', end='')
				print(text_in[pos], end='')
				pos += 1
			pos += 4
			
		elif text_in[pos:pos+3] == '{{{': #code blocks
			pos += 3
			print ('`', end='')
			while text_in[pos:pos+3] != '}}}':
				print(text_in[pos], end='')
				pos += 1
			pos += 3
			print ('`', end='')
			
		elif text_in[pos:pos+2] == '{{': #images
			pos += 2
			start = pos
			while text_in[pos:pos+2] != '}}':
				pos +=1
			parts = (text_in[start:pos].split('|'))
			if len(parts) == 1:
				print('![](' + parts[0] + ')', end='')
			else:
				print('![' + parts[1] + '](' + parts[0] + ')', end='')
			pos += 2
			
		elif text_in[pos:pos+2] == '//': #italics
			pos += 2
			print ('_', end='')
			
		elif mode == 'normal' and text_in[pos:pos+2] == '\n|': #start table
			pos += 2
			mode = 'table header'
			row = 0
			print ('\n<table>\n  <tr>\n    <th>', end='')
		
		elif mode == 'table header' and (text_in[pos:pos+2] == '|\n' or text_in[pos] == '\n'): #newline from table header row
			pos += 3
			mode = 'table'
			print('</th>\n  </tr>\n  <tr>\n    <td>', end='')
		
		elif mode == 'table' and (text_in[pos:pos+2] == '|\n' or text_in[pos] == '\n'): #newline from table row
			pos += 2
			if text_in[pos] != '|':
				print('</td>\n  </tr>\n</table>\n', end='')
			else:
				pos += 1
				print('</td>\n  </tr>\n  <tr>\n    <td>', end='')
			mode = 'normal'
		
		elif mode == 'table header' and text_in[pos] == '|': #next table header column
			pos += 1
			print('</th><th>', end='')
		
		elif mode == 'table' and text_in[pos] == '|': #next table column
			pos += 1
			print('</td><td>', end='')
		
		elif text_in[pos:pos+2] == '\n=' or text_in[pos:pos+3] == '\n =': #header detection -- a little shoddy, only checks leading =.
			pos += 1
			headcount = 0
			while text_in[pos] == '=' or text_in[pos] == ' ':
				if text_in[pos] == '=':
					headcount += 1
				pos +=1
			headcounter = headcount
			while headcounter > 0:
				print('#', end='')
				headcounter -=1
			print(end=' ')
			while text_in[pos] != '=' and text_in[pos] != '\n':
				print(text_in[pos], end='')
				pos += 1
			pos += headcount
			
		else: #must be just text, print it normally
			print(text_in[pos], end='')
			pos += 1
			
	print('\n')
	
if __name__ == "__main__":
	main("""
<< PASTE WIKI PAGE HERE >>
""")