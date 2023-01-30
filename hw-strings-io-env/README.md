# Strings, I/O, and Environment

The purpose of this assignment is to help you better understand strings, I/O,
and environment in C with a series of hands-on exercises.  You will flesh out
sections of an existing C program and answer questions about its output.


# Preparation

Read the following in preparation for this assignment:

 - 10.1 - 10.4 and 10.9 in the book

Additionally, man pages for the following are also referenced throughout the
assignment:

 - `write()`
 - `charsets`
 - `ascii`
 - `printf(3)`
 - `fprintf()`
 - `fputs()`
 - `strcmp()`
 - `memcmp()`
 - `memset()`
 - `strpcy()`
 - `sprintf()`
 - `stdin`
 - `stdout`
 - `stderr`
 - `fileno()`
 - `open()`
 - `fopen()`
 - `read()`
 - `close()`
 - `getenv()`


# Instructions

For each of the sections that follow, follow the instructions, and answer the
questions.  For most questions, you will want to re-compile and re-run the
program _after each question_:

```bash
$ gcc -o learn_c learn_c.c
$ ./learn_c test.txt
```

The point is to learn concepts by seeing them in action along the way.

Note that several exercises will have you modify the command line that you use
to get different results.


# Introduction - Characters, Encoding, and Presentation

Here is very brief lesson on characters, encoding, and presentation.


## ASCII

All char/byte values in C are simply 8-bit integers.  It is the interpretation
and presentation of those integer values according to some character set that
makes them "readable" characters.  For example, byte values less than 128
(i.e., without the most significant bit set) can be interpreted as part of the
American Standard Code for Information Interchange (ASCII) character set, which
includes the upper- and lower-case letters (A-Z and a-z), numbers (0-9),
punctuation, and more.  If these byte values are sent to the terminal, or any
other application that understands them to be ASCII, then it will print out the
ASCII equivalents.  For example, the following array of bytes:

```c
char s1[] = { 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x0a };
```

together represent the ASCII values: `'h'`, `'e'`, `'l'`, `'l'`, `'o'`, `'\n'`
(newline).

If you write them to the terminal, it will display them as such:

```c
write(STDOUT_FILENO, s1, 6);
```

(More will be explained about the `write()` system call later on.  For now,
just know that this system call sends these bytes to the terminal.)


## Beyond ASCII: Unicode

Here is a bit of bonus information related the limitations of ASCII and how
they are addressed.

ASCII has limitations in that it only supports 127 unique characters
(2<sup>7</sup> - 1).  Thus, other character sets have been developed, the most
widely used of which is Unicode.  Unicode has the ambitious goal of having a
mapping for "every character in every human language" (man page for
`charsets`).  ASCII is a subset of Unicode: if a byte value is less than 128,
then it is ASCII, but if it greater, then then it is encoded for Unicode,
typically using a byte encoding called UTF-8.  For example, the following array
of six bytes together represent the two simplified Chinese characters that mean
"Taiwan", followed by an (ASCII) newline:

```c
char s2[] = { 0xe5, 0x8f, 0xb0, 0xe7, 0x81, 0xa3, 0x0a };
```

If you write them to the terminal, again the terminal will display them as
such:

```c
write(STDOUT_FILENO, s2, 7);
```

Finally, take a look at the presentation of the following sequence of
UTF-8-encoded Unicode characters:

```c
char s3[] = { 0xf0, 0x9f, 0x98, 0x82, 0x0a };
write(STDOUT_FILENO, s3, 5);
```

Yup, that is a "joy" emoji.  Not exactly "human language", but you get the
idea.  If it doesn't display in the terminal, try copying it and pasting it in
another application that has the proper support to display it.

For more information, see the man pages for `charsets` and `ascii`.


## `printf()` and friends

What do `printf()` and `fprintf()` do?  There are three things different than
calling `write()`.
 - First, `printf()` and `fprintf()` operate on file streams (`FILE *`), which
   include user-level buffering.  That simply means that they don't `write()`
   until it's efficient to do so.
 - Second, instead of explicitly setting the number of bytes to send,
   `printf()` and `fprintf()` know when to stop sending bytes when they detect
   a null byte value (integer value 0), which you will see in a later exercise.
 - Third, and most importantly for now, they perform "replacements" before
   calling `write()` on the resulting string.  For example, the `s` (e.g.,
   `"%s"`) conversion specifier indicates that it should be replaced by the
   null-terminated string specified.  The `x` and `d` conversion specifiers
   indicate that they should be replaced with the integers corresponding to the
   ASCII characters for the hexadecimal and decimal representations of those
   integers, respectively (See the man page for `printf(3)`).  For example, for
   the conversion specifier `d`, the integer 42 would become the (decimal) byte
   values 52 and 50 (hexadecimal 0x34 and 0x32), corresponding to the ASCII
   characters `'4'` and `'2'`.  After replacements, the modified set of bytes
   is sent to the terminal or application, so "42" is what is presented.

The following snippets all yield equivalent results:

```c
printf("hello %d\n", 42);
```

```c
fprintf(stdout, "hello %d\n", 42);
```

```c
fputs("hello 42\n", stdout);
```

```c
write(STDOUT_FILENO, "hello 42\n", 8);
```

(Note, however, that `write()` does not have buffering.)


## Summary and Main Points

While you will get more hands on with `printf()` and friends in the exercises
at follow, the most important things are:

 - Text is really just a bunch of integer values that an application (e.g., a
   terminal) knows how to interpret and present a certain way--i.e., as text.
 - `printf()` and friends can be used to format text for it to be presented.


# Part 1

 1. Find the number of bytes/characters allocated on the stack for `s1`
    using the `sizeof()` operator (not `strlen()`!).  Note that `sizeof()` is a
    _compile-time_ operator; that means that the size (i.e., the number of
    bytes allocated) is determined at compile time, before the code ever runs.
    In this case, `sizeof()` accurately reflects the number of bytes allocated
    for (i.e., the "size" of) `s1` because `s1` is explicitly assigned a value
    using a string literal.  The same would not be true for something
    dynamically allocated (i.e., at _run time_) with `malloc()`; it simply
    couldn't be known ahead of time.

    Save the value as `s1_len`, and then print `s1_len` on a line by itself,
    using `printf()`.

    *How does the number of bytes allocated for `s1` compare to the number of
    visible characters in `s1`?*

 2. Find the number of bytes/characters allocated on the stack for `s2` using
    the same methodology as you used for question 1.  Note that although
    an initial value is not explicitly assigned to `s2`, as it is with `s1`,
    the size of the array is explicitly given, so `sizeof()` can again be used.

    Save that value as `s2_len`, and then print `s2_len` on a line by itself,
    using `printf()`.

    *How does the number of bytes allocated for `s2` compare to the declared
    number of bytes for `s2`?*

 3. `memprint()` is a function included right in `learn_c.c`.  It simply prints
    the contents of an array of type `char []`, byte-by-byte, to standard
    output using the designated format.

    Call `memprint()` on `s1` three times using `s1_len`, first showing
    each byte/character value as hexadecimal (i.e., format `"%02x"`), then
    showing each byte/character value as decimal (i.e., format `"%d"`), then
    showing each byte/character value as its ASCII representation (i.e., format
    `"%c"`).  What you are seeing is the representation of the contents of
    `s1`, both as the 8-bit decimal and hexadecimal integer values for each
    byte and as their ASCII equivalent.

    a. *What is the value of the "extra" byte allocated for `s1`?*  (That byte
       value is the very definition of what makes `s1` a "string" in C.)

    b. *What is the ASCII character associated with the hexadecimal value
       0x23?* (Hint: See the man page for `ascii`.)

    c. *What is the hexadecimal value of the ASCII character `z` (lower
       case)?* (Hint: See the man page for `ascii`.)


# Part 2

 4. Print out the address of (i.e., using the `&` operator) of each of the
    variables `s1`, `s2`, and `s3`, as a long unsigned integer in decimal
    format (i.e., format `"%lu"`), each on a line by itself.

    The C compiler will complain that you are passing `char *` where a
    `long unsigned int` was expected.  Usually, that means that you are doing
    something wrong!  For this exercise, you can tell the compiler to simply
    treat the value like a `long unsigned int` by explicitly type-casting it
    as such.  To do so, preface it with `(long unsigned int)`.  Please note
    that this does not change anything with regard to the value of the pointer;
    it merely tells the compiler that you are using it differently.

    Note that this exercise has nothing to do with the actual _value_ of the
    variables, which will be compared in a subsequent question.  Rather, this
    only has to do with the memory addresses of the variables themselves.

    *Do any of the variables have the same address on the stack?  If so,
    which and why?*

 5. Print out the address _referred to_ (i.e., its pointer value) by each of
    the variables `s1`, `s2`, and `s3` as a long unsigned integer in decimal
    format (i.e., format `"%lu"`), each on a line by itself.  Since all these
    variables refer to arrays/strings, you can also think of each referred-to
    addresses as that of the _first byte/character_ in each array/string
    referred to.

    See the comment on type casting from question 4.

    Note that while `s1`, `s2`, and `s3` are _declared_ differently, they
    effectively act the same, in that:

    - When represented as a (long unsigned)  _integer_ value (i.e., format
      `"%lu"`), `printf()` uses the referred-to address (i.e., the _pointer
      value_) as a replacement.
    - When represented as a _string_ value (i.e., format `"%s"`), `printf()`
      uses the values _at_ the referred-to address (i.e., the string contents)
      as a replacement.

    However, one difference between `char[]` and `char *` is that for `char[]`,
    the address of the variable is _also_ the address _referred to_ by the
    variable.  That means that there is no changing the _referred-to address_
    (or _pointer value__) of a variable declared `char[]`.

    a. *For any of the variables, is the referred-to address (i.e., the pointer
       value) the same as the _address_ of the variable itself (i.e., the
       answer to question 4)?  If so, which and why?*  Hint: See explanatory
       text in this question.

    b. *Do any of the variables reference the same content?  That is, are any
       of the addresses / pointer values the same between `s1`, `s2`, and/or
       `s3`?  If so, which and why?*

 6. Use `printf()` to print out the contents of each of the array/string
    variables `s1`, `s2`, and `s3`, i.e., using the `"%s"` format, each on a
    line by itself.

    *Which pairs of arrays/strings have the same content and why?*

 7. Compare the following pairs of pointer values using the equality operator,
    `==`: `s1` and `s2`; `s1` and `s3`; `s2` and `s3`.  In each case, print
    "s1 == s2" (replacing the variable names, as appropriate) on its own line
    if the values are equal and "s1 != s2" otherwise.

    *Which pairs of arrays/strings have the same pointer values and why?*

 8. Compare the values of the strings referenced by the following pairs of
    pointers, using the `strcmp()` function: `s1` and `s2`; `s1` and
    `s3`; `s2` and `s3`.  In each case, print "s1 == s2" (replacing the
    variable names, as appropriate) on its own line if the values are equal and
    "s1 != s2" otherwise.

    *Which pairs of arrays/strings have the same content and why?*
    (Hint: Answer the same as for question 6).

 9. Note the existing code immediately before question 9. Now repeat the
    instructions for question 6.

    *For which arrays/strings has the content changed from question 6,
    and why?*

 10. Repeat the instructions for question 7.

     *Has the equivalence of pointer values changed from question 7?  Why or
     why not?*

 11. Repeat the instructions for question 8.

     *Has the equivalence of content changed from Question 8?  Why or why not?*


# Part 3

 12. Compare the values of the bytes referenced by the following pairs of
     pointers, using the `memcmp()` function (not `strcmp()`!):
     `s1` and `s2`; `s1` and `s3`; `s2` and `s3`.  In each case, print
     "s1 == s2" (replacing the variable names, as appropriate) on its own line if
     the values are equal and "s1 != s2" otherwise.

     *Which pairs of arrays/strings have the same content and why?*
     (Hint: See both the Intro and question 3).


# Part 4

 13. Compare the values of the bytes referenced by pointers `s1` and `s2`,
     using the `memcmp()` function (not `strcmp()`!).  Print "s1 == s2" on its
     own line if the values are equal and "s1 != s2" otherwise.

     *Does `memcmp()` indicate that the arrays have the same content?  Why
     or why not?*

 14. Compare the values of the bytes referenced by pointers `s1` and `s2`,
     using the `strcmp()` function (not `memcmp()`!).  Print "s1 == s2" on its
     own line if the values are equal and "s1 != s2" otherwise.

     *Does `strcmp()` indicate that the arrays have the same content?  Why
     or why not?*

 15. Use `memset()` to initialize every byte value in `s3` to `'z'` (or,
     equivalently, `0x7a`).  Then call `memprint()` on `s3` to show the
     hexadecimal value of each byte/character (i.e., format `"%02x"`).

     Repeat both the `memset()` function and the `memprint()` function for
     `s4`.

     *Does the output show any characters other than (ASCII) `z`?* (Hint: It
     shouldn't.)

 16. Use the `strcpy()` function (not `memcpy()`!) to copy the contents of `s1`
     to `s3`.  Then call `memprint()` on `s3` to show each byte/character value
     in the array as hexadecimal (i.e., format `"%02x"`).

     *Which bytes/characters were copied over from `s1`?*

 17. Use the `sprintf()` function to replace the contents of `s4`.  Use the
     format string `"%s %d\n"` and the values of `s1` and `myval`. Then call
     `memprint()` on `s4` to show each byte/character value in the array as
     hexadecimal (i.e., format `"%02x"`).

     The C compiler will complain that the number of characters that will be
     used to represent the integer (i.e., `%d`) could possibly exceed the size
     of the buffer.   This is true.  A signed 32-bit integer could be as low as
     -2147483648 (11 digits) or it could be 0 (1 digit). In the former case, it
     would overflow the buffer we have allocated.  For the purposes of this
     assignment, please ignore the warning.  We have hard-coded the value of
     `myval`, so we know the number of characters the will be needed to
     represent `myval` in replacing `%d`.  In a real program, however, please
     use more care.

     *In which places of the array was a null value placed?*


# Part 5

 18. Read the man page for `stdin` (which also shows the information for
     `stdout` and `stderr`).  Now use the `fileno()` and `printf()` functions
     to find and print out the file descriptor values for the `stdin`,
     `stdout`, and `stderr` file streams, each on a line by itself.  For
     example: "stdout: n" (where `n` is the descriptor value).

     *What are the file descriptor values for stdin, stdout, and stderr?*

 19. Use `memset()` to initialize every byte value in `buf` to `'z'` (or,
     equivalently, `0x7a`).  Then assign the byte at index 24 to `'\0'` (or,
     equivalently, 0).  Finally, call `memprint()` on `buf` to show the
     hexadecimal value of each byte/character (i.e., format `"%02x"`).

     *Does the output show any characters other than (ASCII) `z` and null?
     (Hint: It shouldn't.)*

 20. Print out the contents of `buf` to standard output in two ways:

     1. call `printf()` using the `"%s"` format string;
     2. call `write()`, specifying the integer file descriptor value for
        standard output; in this case, use `BUFSIZE` as the byte count.
        (Hint: see the
        [introduction section](#introduction---characters-encoding-and-presentation)).

     After each, print a newline character, so each printout is on its own line.

     *Is there a difference between what was printed by `printf()` and what was
     printed by `write()`?  Why or why not?*  (Hint: See the `s` Conversion
     Specifier in the man page for `printf(3)`.)

 21. Print out the contents of `buf` to standard error (not standard out!) in
     two (new) ways:

     1. call `fprintf()`;
     2. call `write()`, specifying the integer file descriptor value for
        standard error; in this case, use `BUFSIZE` as the byte count.

     After each, print a newline character, so each printout is on its own line.

     *What happens when you append `> /dev/null` to the end of the command
     line?*

 22. Using the `open()` system call (not `fopen()`), open the file specified by
     the filename variable passing `O_RDONLY` as the only flag (i.e., open the
     file for reading only).  Save the return value as an integer variable,
     `fd1`.  Now copy that value to another integer variable, `fd2`.  Print out
     the values of `fd1` and `fd2`, each on its own line.

     *What is the significance of the value of `fd1`, i.e., the return value of
     `open()`?*
     (Hint: See the first two paragraphs of the DESCRIPTION for `open(2)`.)

 23. Use the `read()` system call to read up to 4 bytes from `fd1`, placing the
     bytes read into `buf`.  Save the return value as `nread`.  Add the value
     of `nread` to `totread`.  Then print the values of `nread` and `totread`,
     each on their own line (they should currently be the same).  Finally, call
     `memprint()` on `buf` using `BUFSIZE` showing each byte/character value as
     hexadecimal (i.e., format `"%02x"`).

     a. *Did the return value from `read()` match the count value passed in?
        Why or why not?* (Hint: See the RETURN VALUE section in the man page
        for `read(2)`.)

     b. *Was a null character included in the bytes read or immediately
        following them?  Why or why not?*  (Hint: To answer the "why" question,
        use the `hexdump` command-line utility to inspect the contents of
        `test.txt`.)

 24. Use the `read()` system call to read up to 4 bytes from `fd2` (not
     `fd1`!).  Instead of using `buf` as the starting point at which the read
     data should be copied, use the offset into `buf` corresponding to the
     total bytes read (i.e., `buf + totread` or `&buf[totread]`). Save the
     return value as `nread`. Add the value of `nread` to `totread`.  Then
     print the values of `nread` and `totread`, each on their own line.
     Finally, call `memprint()` on `buf` using `BUFSIZE` showing each
     byte/character value as hexadecimal (i.e., format `"%02x"`).

     a. *Did this new call to `read()` start reading from beginning of the file
        or continue where it left off after the last call?  Why?*
        (Hint: See the RETURN VALUE section in the man page for `read(2)`.)

     b. You have now used two variables, in two different calls, to read
        from the file.  *Based on your answer to a, does the _address_ of
        the variable referencing a file descriptor matter, or only its
        _value_?*

     c. *How many total bytes have been read?*

 25. Repeat the instructions for question 24, but this time read up to 50
     bytes, instead of 4.

     a. *Did the return value from read() match the count value passed in?  Why
        or why not?*
        (Hint: See the RETURN VALUE section in the man page for `read(2)`.)

     b. *How many total bytes have been read?*

     c. *How many total bytes are in the file?*
        (Hint: Use the `stat` command-line utility to see the size of the file,
        in bytes.)

 26. Repeat the instructions for question 25.

     *What is the significance of the return value of `read()`?*
     (Hint: See the RETURN VALUE section in the man page for `read(2)`.)

 27. Using the `fopen()` function (not `open()`!), open the file specified by
     the filename variable passing `"r"` as the mode (equivalent to `O_RDONLY`
     with `open()`). Save the return value as a file stream (`FILE *`).  Now
     use the `fileno()` and  `printf()` functions to find and print out the
     file descriptor values for the newly returned file stream.  Finally, close
     the file stream using `fclose()`.

     a. *What is the file descriptor value for the new file stream?*

     b. *What is the significance of the file descriptor value of the new file
        stream?*
        (Hint: See the first two paragraphs of the DESCRIPTION in the man page
        for `open(2)`.)

 28. Call `close()` on `fd1`, and use `printf()` to print the return value on a
     line by itself.  Then call close on `fd2`, and use `printf()` to print the
     return value on a line by itself.

     a. *What is the return value of the first instance of `close()`?  What does
        this mean?*
        (Hint: See the RETURN VALUE section in the man page for `close(2)`.  See
        also question 24b.)

     b. *What is the return value of the second instance of `close()`?  What does
        this mean, and what is the likely cause?*
        (Hint: See the RETURN VALUE section in the man page for `close(2)`. See
        also question 24b.)


# Part 6

 29. Use `getenv()` to assign the pointer `s1` to the string corresponding to
     the environment variable `CS324_VAR`.  If such a value exists, then print:
     `"The value is: _____\n"` (replace `_____` with the actual value);
     otherwise, print `"CS324_VAR not found\n"`.

     Run your code with and without the `CS324_VAR` environment variable set
     and with different values.

     *What command line did you use to assign the environment variable
     `CS324_VAR` the value of "awesome" (without quotes) such that it could be
     retrieved by your program when it is run?* (Hint: `FOO=var ./cmd` sets the
     value of the environment variable `FOO` to `var` for the single running
     instance of `cmd`.)
