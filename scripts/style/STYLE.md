# Code Style Notes

This file contains general notes about formatting and coding style.

## General Notes

We use spaces everywhere, with a single indentation level of 4 spaces.
All tools are configured to convert and get rid of the tabs.

The tool used for formatting is uncrustify, with a long list of common
options for formatting that are shared between different languages,
as well as language-specific options.

We use 120 as the max allowed line length. Traditional 80 is way
too small, but we do want to enforce some max.
120 seems like a good compromise.

Formatter is configured to try to respect the max length of the lines,
but it is possible that it violates it. It is also possible
that it breaks the code to make the lines shorter,
but in a way that looks weird or unreadable.
It is the programmer's responsibility to break lines like that
in a way that makes sense.

Similarly, sometimes auto-formatter will not generate code that
is readable. It is poissible especially when comments and macros
are involved. Again, it is a responsibility of the programmer
to modify the code so that the auto-formatter formats it in a way
that makes sense.

## Known Quirks

Here are some quirks that we stumbled upon, and suggestions
how to deal with them.

### Long lines with nested calls

If there is a function call that includes other calls, especially
creatign new objects at nested levels, it can get pretty long.
If the formatter tries to wrap lines like that it may choose
to wrap it at the lowest level - resulting in a long line, with
several short elements wrapped and indented at the end of it.

The right solution is just to break that line manually
in places where it makes more sense and have the formatter
figure out the indentation later.

### Comments in between if-else blocks

The formatter is configured to leave empty lines before the comments.
This helps readibility and usually is a good practice.

However, if the comment is used after an 'if' block and before 'else',
those empty lines will be confusing.

For example, a piece of code may end up looking like that
(with an empty line before else after formatting):

```C
// Foo is true
if ( foo )
{
  ...
}

// Foo is false - do bar():
else
{
  bar();
}
```

The solution is to move the comments into their respective blocks:

```C
if ( foo )
{
  // Foo is true
  ...
}
else
{
  // Foo is false - do bar():
  bar();
}
```
