# readini Project

The **readini** project is a simple-to-use library of functions
designed to return tagged values from a configuration file.

The genesis of this project is, perhaps, a little unusual.  Read
about it near the bottom of this page.

## Compile and Install

This project is simple enough that I am not including a **configure**
script, only a **Makefile**.  Make and install like this:

~~~sh
$ git clone https://www.github.com/cjungmann/readini.git
$ cd readini
$ make
$ sudo install
~~~


















## Purpose of Project

While there are many reasons to use configuration files, the
inspiration for me was in designing command-line tools that
need private information for testing.  I don't want to have to
repeatedly enter several command line options for each test,
and I don't want to put private information into a testing
script.

Using this tool, I can create a configuration file with my
private information and *.gitignore* it to keep it out of
my repository.



~~~sh
[global]
mailhost : smtp.gmail.com
mailport : 587

[colons]
user : bogus@gmail.com
from : bogus@gmail.com
password : bogus

[spaces]
user sham@gmail.com
from sham@gmail.com
password sham_word

[equals]
user = phony@gmail.com
from = phony@gmail.com
password = phony_secret

~~~