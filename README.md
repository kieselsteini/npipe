Net Pipe
========

npipe the network/names pipe utility.

Summary
-------

npipe provides FIFO (named pipes) to communicate with remote servers.
You can invoke npipe to connect to a remote host and it will provide an "in"
and one "out" file. Data placed in "in" will be transferred to the host. All
data received from the remote host will be placed in out. You can use bash
or other programs to evaluate the output and generate new input just by
reading/writing to files.

Example
-------

A simple POP3 test...
```shell
npipe -h pop3.mailhost.org -p 110 -f
echo "USER myaccount" > in
echo "PASS mypassword" > in
echo "LIST" > in
cat out
```

out should now contain the emails you have on the server.


Usage
-----

npipe requires no configuration but some command line arguments to work
properly. Here's a list of all command line arguments you can use:
 * -v: prints version and exits
 * -V: enable verbose mode (print incoming/outgoing data to stdout)
 * -f: forks npipe after start and prints PID to stdout
 * -h [hostname]: the hostname npipe should connect to
 * -p [port]: the port npipe should use to connect


Bugs
----
 * no bugs I'm aware of...feel free to find some


Todo
----
 * only data with newlines at the end will be sent at once
 * npipe waits for newlines from the host
 * a binary mode ?!

I hope this small tool is useful for you. Send me a mail if you have
suggestions or patches.

