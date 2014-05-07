baren
=====

Command-line Batch File Renamer

Replaces all occurrences of the pattern in regex with the replacement text. Backreferences of the form '\number' or '\g<number>' in the replacement text are interpolated by the number-th captured subexpression of the match, '\g<name>' refers to the captured subexpression with the given name. '\0' refers to the complete match, but '\0' followed by a number is the octal representation of a character. To include a literal '\' in the replacement, write '\'.

There are also escapes that changes the case of the following text:

\l: Convert to lower case the next character

\u: Convert to upper case the next character

\L: Convert to lower case till \E

\U: Convert to upper case till \E

\E: End case modification
