##TINY HTTP SERVER
A simple httpd based on POSIX socket that currently supports http1.0/1.1 GET. (Other commands are under construction)

###HOW TO INSTALL and COMPILE
1. unzip the compressed file and cd into the folder
2. make

###HOW TO RUN
Server:
[command]
myhttpd <http version> <port number> <timeout>
myhttpd_thread <http version> <port number> <timeout>

[arguement requirements]
<http version>: 1/1.1
<port number>: >1024 and <65536
<timeout>: 0 for http 1.0, and positive integer for http 1.1


Client:
[command]
clg <server> <portnumber> <httpversion> <threads/processes number>

[arguement requirements]
<server>: server address
<port number>: >1024 and <65536
<httpversion>: 1/1.1
<threads/processes number>: number of threads or processes

