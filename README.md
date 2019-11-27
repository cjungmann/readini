# readini Project

The **readini** project is a simple-to-use library of functions
designed to return tagged values from a configuration file.

The genesis of this project is, perhaps, a little unusual.  Read
about it near the bottom of this page.

## Using the Library Functions

Before you download, compile, and install this library, you'll
want to know if it fits your needs.  Therefore, I'll show you
how it works first.  If you like what you see, look further down
for instructions about how to set up the library.

### Easiest Method

The fundamental processing is done with two functions:

- **ri_read_file** reads the entire configuration file and makes
  its contents available in a linked list.
- **ri_find_section_value** scans the linked section list for
  the appropriate line, returning the value associated with the
  tag.

~~~c
#include <stdio.h>
#include <readini.h>

void use_sections(const ri_Section* sections)
{
   const char *user = ri_find_section_value(sections, "bogus", "user");
   const char *password = ri_find_section_value(sections, "bogus", "password");
}

int main(int argc, char** argv)
{
   ri_read_file("./mail.conf" use_sections);
   return 0;
}
~~~

### Slightly-harder Method

For long configuration files from which a small amount of data
is needed, it may be more efficient to open the file and read
only the necessary sections.  In that case, the following three
functions can be used:

- **ri_open** to open the file.
- **ri_open_section** returns a linked-list of lines in a
  named section.
- **ri_find_value** is a simple function that scans the
  linked list for a matching tag and returning the associated
  value.

~~~c

struct rdata
{
   const ri_Line *global;
   const ri_Line *account;
};

void read_account(int fh, const ri_Line* lines, void* data)
{
   struct rdata *status = (struct rdata*)data;
   status->account = lines;

   // Use the global and account settings to do your work.
}

void read_global(int fh, const ri_Line* lines, void* data)
{
   struct rdata status = { lines, NULL };
   const char *acct_name = ri_find_value(lines, "default-account");
   if (acct_name)
      ri_open_section(fh, acct_name, read_account, (void*)&status);
}

void use_file(int fh, void* data)
{
   ri_open_section(fh, "global", read_global, NULL);
}

int main(int argc, char** argv)
{
   ri_open("./mail.conf", use_file, NULL);
   return 0;
}
~~~

### Life-time of Linked Lists

For any function that returns a linked list, (**ri_open_section**
and **ri_read_file**), the linked list remains in memory until
the callback function returns.  That means that values returned
from functions **ri_find_value** and **ri_find_section_value**
are valid until the callback function returns.

The linked lists are implemented in stack memory, so the contents
are likely to be immediately corrupted by continued execution of
your program.

### Configuration File Format

The configuration file will contain sections indicated by a
name enclosed in square brackets.  Each section is followed
by one or more lines of solitary tags, or tags followed by
values.

The tag is string without spaces.  If the tag is assigned a
value, it is followed by a second string, separated by any
combination of spaces, colons or equal signs.  In practice,
you'll select one separator format.

## Compile and Install

This project is simple enough that I am not including a **configure**
script, only a **Makefile**.  Make and install like this:

~~~sh
~/ $ git clone https://www.github.com/cjungmann/readini.git
~/ $ cd readini
~/readini $ make
~/readini $ sudo install
~~~

## Test Install

There is a small subproject in this project, **ritest**.  Compiling
the source file, **ritest.c** with the appropriate flags will
confirm if the installation was successful.

~~~sh
~/readini $ cc -o ritest ritest.c -lreadini
~/readini ./ritest
~~~

If **ritest** runs without error, showing the contents of
the section *[bogus]*, then the installation is correctly
installed.


## Include Library in Your Project

Using a shared-object library makes it easy to add to your
project.  Simply add the new header file to your source code
and the library link instruction to your **make** file:

~~~c
// file ritest.c
#include <readini.h>
~~~


~~~make
# file Makefile
CFLAGS = -Wall -m64 -ggdb -lreadini
~~~

## Testing the Library

I included a sample program in **ritest.c** that is setup
to read a sample configuration file, **demo.ini**.  You
can look at **ritest.c** for hints on how to use the library,
and **demo.ini** for an example of what variations of lines
can be read.


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