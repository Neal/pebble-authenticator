#!/usr/bin/env python

import base64
import sys

secrets = []
labels = []
lengths = []
time_zone = "0.0"

def genKeyLine(code):
	secret_key = code.replace(' ','').upper()
        pad = 0 if (len(secret_key) % 8) == 0 else 8 - (len(secret_key) % 8)
        key_enc = secret_key + ('='*pad)
        key = base64.b32decode(key_enc)
	key_bytes = map(ord,key)
	lengths.append(len(key_bytes))
	key_hex = ["0x%02X" % x for x in key_bytes]
	return '{ ' + ', '.join(key_hex) + ' },'

def generate():
	try:
		f = open('configuration.txt', 'r')
	except:
		print 'Unable to open configuration.txt. Cheack README.md for configuration details.'
		sys.exit(1)
	
	for line in f:
		line = line.strip()
		if line.startswith('#') or not ':' in line: continue
		key,value = line.split(':')
		if (key.lower() == 'tz'):
			time_zone = value
		else:
			labels.append(key)
			secrets.append(genKeyLine(value))
	f.close()
	
	f = open('src/configuration.h', 'w')
	
	f.write("#ifndef _CONFIGURATION_H_\n")
	f.write("#define _CONFIGURATION_H_\n\n")
	f.write("#define DEFAULT_TIME_ZONE %s\n" % time_zone)
	f.write("#define NUM_SECRETS %i\n\n" % len(labels))
	f.write("char otp_labels[NUM_SECRETS][17] = {\n\t")
	for label in labels:
		f.write("\"%s\"," % label)
	f.write("\n};\n\n")
	f.write("unsigned char otp_keys[NUM_SECRETS][%s] = {\n" % max(lengths))
	for secret in secrets:
		f.write("\t%s\n" % secret)
	f.write("};\n\n")
	f.write("int otp_sizes[NUM_SECRETS] = {")
	for length in lengths:
		f.write("%s," % length)
	f.write("};\n\n#endif\n")
	
	f.close()

# generate configuration.h if this file is run as the main script
if __name__ == '__main__':
	generate()
