Baren
=====

Baren is a Command-line Batch File Renamer. It allows quick mass-renaming of files through the use of Perl-compatible regular expressions.

This means you can, for instance, easily rename all your *.avi* files to *.mkv*, without the risk of overwriting a file with another, but you can actually do much more, such as renaming all of your *something-01_whatever_02.xyz* files to *01-02.txt* in a pinch.

Be aware that Baren uses *regular expressions*, not simple MS-DOS/shell-like patterns. If you don't know what a regular expression is, you can refer to http://www.regular-expressions.info, for instance. You can also use http://regex101.com to create and refine regular expressions so that they match exactly as you want them.


# Usage
Keep in mind that, by default:
* **Baren will process files in the CURRENT DIRECTORY**. Use the *-d* switch if you would like to work in a different path.
* **Baren will NOT rename any files**, it will just show a preview of the changes it will perform, so use it without fear. You will need to use the *-a* and *-A* flags to actually perform any changes.
* **Baren will NOT overwrite any existing files, or any files that it has just renamed during the current run**. Files of the former kind can be overwritten if you specify the *-f* flag, while files of the latter kind will **NEVER** be overwritten.
* **Baren will NOT rename directories**, only files. Use the *-n* flag is you want to rename directories as well.
* **Baren will NOT recurse into subdirectories**. Use the *-r* flag if you want to do so.
* **Baren will NOT match patterns on file extensions**, so that they will always be preserved. If you want that, use the *-e* flag.
* **Baren will match patterns in a CASE-SENSITIVE fashion**. Use the *-i* for expressions to be case-insensitive.

# Examples:
* Strip an initial string:
````
baren '^.+ ITA - (.+)' '\1'
````
* Rename, without worrying about upper or lower case:
````
baren -i '.+(\d+x\d+)\s+(.+) ITA.+' '\1. \2'
````
* Rename all **.avi** files to **.mkv**:
````
baren -e '(.+)\.avi$' '\1.mkv'
````
Notice the use of the *-e* flag, so that the pattern gets matched on the file extension.

# Notes
Baren is a glib-based application, so the exact syntax of the regular expressions it supports can be found at https://developer.gnome.org/glib/stable/glib-regex-syntax.html.

This also allows for escape sequences that changes the case of the following text:

* **\l**: Convert to lower case the next character
* **\u**: Convert to upper case the next character
* **\L**: Convert to lower case till \E
* **\U**: Convert to upper case till \E
* **\E**: End case modification


## Renaming files
Renaming files is usually a straightforward operation, but it has some system-dependent characteristics that you can read in the *man* page for the *rename()* system call of your system, i.e.:
````
man 2 rename
````

One thing that you should b aware of is that When renaming a directory, the new name must either not exist, or it must specify an empty directory.

# License
**Baren is copyright (C) 2014 by SukkoPera <software@sukkology.net>.**

Baren is **free software**: you can redistribute it and/or modify it under the terms of the **GNU General Public License** as published by the Free Software Foundation, either **version 3** of the License, or (at your option) any later version.                  
