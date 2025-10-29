# Steganography tools

Tools for steganography.

### img\_steg.py
* Python image steganography tool (encode and decode using lsb method)
* Requires Python3 with the PIL and numpy libraries

### png\_steg.c
* PNG image steganography tool (encode and decode using lsb method)
* Build with command **"gcc -o png\_steg png\_steg.c -lpng"**

Both tools will terminate the encoded data with a null byte. Decoding will stop when it gets a null byte.  
Binary data will need to be base64 encoded in order to encoded with those tools.

