# pgcodec7
When encoding byte array into text, one usually group them into 4 resulting hexadecimal character. The encoded size will be twice byte array size.
By grouping into 7 or 6 bits, we well have less encoded size.

This project is compiled in Linux against PostgreSQL version 11.<br />
$ make clean<br />
$ make<br />
$ make install<br />

On successful compilation, install this extension in PostgreSQL environment<br />
$ create extension pgcodec7<br />

Let us encode byte array from file -let us say- '/tmp/sample.jpg' with 7 bits grouping.<br />
$ select * from pgencode7(7, '/tmp/sample.jpg');<br />
or for 6 bits grouping
$ select * from pgencode7(6, '/tmp/sample.jpg');<br />
You will have set of text. Due to limitted buffer size of 1024, the text result is separated into one or more lines.
To decode, concatenate the text lines then supply it to pgdecode7:

$ select pgdecode7(7, '....<encoded text>');<br />
or
$ select pgdecode7(6, '....<encoded text>');<br />
And you will have byte array back.

