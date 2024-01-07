.ig
Insert a heading.  If given one argument, that argument is the heading
text and the heading level is assumed to be 1.  If given two arguments,
the first argument is the heading level and the second is the text.  This
is mostly like .NH but it sets the font size to some nice values.
..
.de H
.ie \B'\\$1' \{\
.	nr arg \\$1
.	ds title \\$2
.\}
.el \{\
.	nr arg 1
.	ds title \\$1
.\}
.nr ps_ \n[PS]
.if (\\n[arg] == 1)
.	nr PS \\n[ps_]+3
.if (\\n[arg] == 2)
.	nr PS \\n[ps_]+2
.if (\\n[arg] == 3)
.	nr PS \\n[ps_]+1
.NH \\n[arg]
.nr PS \\n[ps_]
.XN \\*[title]
..
.
.ig
Start and end a source-code block.  When compiling to a PDF these
blocks are surrounded by a box.  When reading in a terminal, a simple
indent should be enough.
..
.de SC
.if '\*[.T]'pdf' \{
.	B1
.	ft C
.\}
.DS
.
..
.de SE
.DE
.if '\*[.T]'pdf' \{
.	ft
.	B2
.\}
..
.
.\" Space out paragraphs a bit more
.nr PD 1v
.
.\" Remove automatic page-number header
.ds CH
.
.TL
The Andy Shell
.AU
Thomas Voss
.LP
This is the official reference manual of the Andy shell language.
It is a complete description of the language syntax, -features,
and -builtins,
and also documents certain style preferences and language idioms.
If any mistakes are found or information is missing,
feel free to contact
.I mail@thomasvoss.com
or to submit a patch to the official Git repository at
.I https://git.sr.ht/~mango/andy .
.bp
.
.\" Begin counting pages
.nr % 1
.ds RF [Page %]
.
.
.H "Prologue"
.H 2 "What is Andy?"
.LP
The Andy shell is a modern shell that aims to do away with the
deficiencies standardized by POSIX.
It does not aim to replace POSIX-compliant shells such as Dash or Mksh as
a /bin/sh implementation.
Andy instead targets the domain of interactive shell use, and
non-portable scripting;
while it’s unlikely that the world will move on from POSIX anytime soon,
there is no reason for most users’ personal scripts to strictly follow
POSIX.
The Andy shell provides a safer and more powerful shell interface than
the common Bourne Shell,
while still remaining small, simple, and close to the UNIX spirit.
.
.LP
This shell is dedicated to Andy Zaidman.
So much so in fact, that the shell is named after him.
.
.
.H 2 "Conventions In This Document"
.LP
This document will contain various code examples.
If an example begins with a shebang (‘#!/usr/bin/andy’) then the example
represents a shell script.
In these cases, the first line beginning with ‘⇒’ represents the scripts
output.
When an example instead represents interactive usage,
any lines beginning with the prompt ‘$’ represent a user-entered command,
and any lines without the prompt-prefix represent the output of the
previous user-entered command.
This document may also contain various references to builtin-functions
or external commands,
these will be formatted using a bold fontface such as in
.B echo
and
.B uname .
.
.
.H "Values"
.H 2 "What Are Values?"
.LP
In Andy, a
.I value
is syntactical element which expands to one or more strings.
These strings are then interpreted literally by the shell.
The list
.CW <{foo} ’ ‘
for example is a value which expands to the string ‘/dev/fd/N’
(where ‘N’ is some integer).
This particular value is known as a
.I "process redirection" ,
and will is described later in this document.
Values may be used in any place where a regular argument may be used.
This includes command arguments,
filenames given to redirections,
and even function names.
.
.
.H "Exit Codes and Shell Execution"
.H 2 "Interpreter Behavior"
.LP
In many shells the input file is parsed as it is executed.
That is to say that if an input file contains a syntax error,
it may only be caught at runtime.
Andy takes a different more sensible approach and parses the complete
input file before executing any code.
This allows lexing- and parsing errors to be caught early,
avoiding any potential unwanted behavior.
.
.
.H 2 "Exit Codes"
.LP
All statements in Andy have an
.I "exit code" .
This exit code is an integer in the range of 0–255 and may be inspected
through the magic variable
.CW $status
directly after executing a statement.
As is the UNIX tradition,
an exit code of 0 represents successful execution
while a nonzero exit code represents failure.
Andy will typically use an exit code of 255 to represent a shell-error,
such as failure to open a file for a redirection,
or a syntax error during interactive use.
.LP
Unlike in most traditional shells,
Andy will automatically cede execution at the first nonzero exit code
encountered at runtime.
A failing exit code is seen as an unintended error,
and halting execution is the safest way to proceed.
If you want to explicitly state that a certain potential error is to be
ignored,
you can make use of the
.B true
builtin function.
.SC
#!/usr/bin/andy

printf \(aqhello \(aq
# The false builtin function always fails with an exit code of 1
false || true
echo world

⇒ hello world
.SE
.LP
In the above example the exit code of
.B false
is ignored by chaining it with
.B true
in a
.I "conditional chain" .
These will be covered in greater depth later.
.
.
.H "Commands"
.H 2 "Simple Commands"
.LP
Simple commands are the simplest form of statement available in Andy.
A simple command consists of one or more values,
and is terminated by either a newline or a semicolon.
If no value is provided, nothing happens.
Much like in a traditional POSIX-compliant shell,
a simple command invokes the specified program with the specified
arguments.
.SC
$ cat file
hello world
$ echo hello; echo bar
hello
world
$ echo <{true}
/dev/fd/3
.SE
.
.LP
A simple command may optionally be followed by one or more redirections.
Unlike in POSIX-compliant shells,
redirections are not permitted at any position other than the end of the
command.
Spaces are permitted on both sides of the redirection operators,
however it is considered good style to put no space after the redirection
operator\*[*].
.SC
$ echo hello, world! >foo
$ tr a-zA-Z <foo
HELLO, WORLD!
.SE
.
.LP
Redirections are covered in greater detail later in the document.
.
.FS
This keeps the syntax consistent with process redirections such as
.CW <{foo} ’. ‘
.FE
.
.
.H 2 "Compound Commands"
.LP
In certain situations (such as in a pipeline) you may want to interpret
multiple simple commands as one single unit.
This is possible with compound commands;
a sequence of commands enclosed in braces.
.SC
$ echo foo bar | { read -d\(aq \(aq -n1 x; cat } | tr a-z A-Z
BAR
$ echo $x
foo
.SE
.
.LP
Within a compound command you may include any code that you can input
outside of a compound command,
such as loops and conditional statements.
The compound command merely groups them together into one unit.
This is also useful for applying a single redirection onto multiple
commands.
.SC
#!/usr/bin/andy

{
	printf \(aqhello \(aq
	echo world
} >foo
cat foo

⇒ hello world
.SE
.
.
.H "Comments"
.LP
As is typical with programming languages in the UNIX environment,
comments begin with a pound sign (‘#’) and continue until the end of the
line.
Comments can be used anywhere and will be completely ignored by the
shells lexer.
This includes within pipelines — a feature not available in the POSIX shell.
.SC
#!/usr/bin/andy

printf %s\en bar foo bar
# Uppercase input
| tr a-z A-Z
# Sort words
| sort
# Remove duplicate entries
| uniq

⇒ BAR
FOO
.SE
