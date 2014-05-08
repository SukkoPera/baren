/***************************************************************************
 *   Baren - Command-line Batch File Renamer                               *
 *                                                                         *
 *   Copyright (C) 2014 by SukkoPera <software@sukkology.net>              *
 *                                                                         *
 *   Baren is free software: you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Baren is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Baren.  If not, see <http://www.gnu.org/licenses/>.        *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

static gchar *path = NULL;
static gboolean caseless = FALSE;
static gboolean recurse = FALSE;
static gboolean rendirs = FALSE;
static gboolean overwrite = FALSE;
static gboolean verbose = FALSE;
static gboolean apply = FALSE;
static gboolean apply_no_overwrite = FALSE;
static gboolean matchext = FALSE;


// Returns true if no files are overwritten
gboolean process_dir (gchar *path, const GRegex *rx, const gchar *replacement) {
	GDir *dir;
	GError *error;
	GHashTable *renamedFiles;
	gboolean ret;

	g_debug ("Processing directory: \"%s\"", path);

	renamedFiles = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	ret = TRUE;

	error = NULL;
	if (!(dir = g_dir_open (path, 0, &error))) {
		g_printerr ("Cannot get directory listing for \"%s\": %s", path, error -> message);
		g_error_free (error);
	} else {
		const gchar *filename;

		while ((filename = g_dir_read_name (dir))) {
			// '.' and '..' entries are already omitted by the great glib
			gboolean isdir;
			gssize len = strlen (filename);
			gchar *fullfilename = g_build_filename (path, filename, NULL), *lastdot = NULL;

			if ((isdir = g_file_test (fullfilename, G_FILE_TEST_IS_DIR))) {
				// Directory, recurse only if requested
				if (recurse) {
					if (verbose)
						g_print ("dr %s\n", fullfilename);
					ret = process_dir (fullfilename, rx, replacement);
				} else {
					g_print ("ds %s\n", fullfilename);
				}
			} else {
				// File, prepare to skip file extensions if requested
				if (!matchext) {
					lastdot = strrchr (filename, '.');
					if (lastdot) {
						len = lastdot - filename;
					}
				}
			}

			// Always try to match & rename files, but directories only if requested explicitly
			if (!isdir || rendirs) {
				error = NULL;
				gchar *n = g_regex_replace (rx, filename, len, 0, replacement, 0, &error);
				if (strncmp (filename, n, (size_t) len) != 0) {
					// Pattern matched, try to rename
					g_print ("%c- %s\n", isdir ? 'd' : ' ', fullfilename);

					gchar *fullto;
					if (lastdot) {			// Add extension
						gchar *tmp = g_strconcat (n, lastdot, NULL);
						fullto = g_build_filename (path, tmp, NULL);
						g_free (tmp);
					} else {
						fullto = g_build_filename (path, n, NULL);
					}
					if (g_file_test (fullto, G_FILE_TEST_EXISTS)) {
						// File (or director) already exists
						ret = FALSE;
						if (overwrite) {
							if (apply && g_rename (fullfilename, fullto) < 0) {
								g_print ("%cE %s\n", isdir ? 'd' : ' ', fullto);
								g_free  (fullto);
							} else {
								g_hash_table_add (renamedFiles, fullto);
								g_print ("%c* %s\n", isdir ? 'd' : ' ', fullto);
							}
						} else {
							g_print ("%c! %s\n", isdir ? 'd' : ' ', fullto);
							g_free  (fullto);
						}
					} else if (g_hash_table_contains (renamedFiles, fullto)) {
						// File would overwrite an already-renamed file.
						// In this case we never overwrite.
						g_print ("%c$ %s\n", isdir ? 'd' : ' ', fullto);
						g_free  (fullto);
					} else {
						// File does not exist
						if (apply && g_rename (fullfilename, fullto) < 0) {
							g_print ("%ce %s\n", isdir ? 'd' : ' ', fullto);
							g_free  (fullto);
						} else {
							g_hash_table_add (renamedFiles, fullto);
							g_print ("%c+ %s\n", isdir ? 'd' : ' ', fullto);
						}
					}
				} else {
					// Pattern did not match
					if (verbose)
						g_print ("%c# %s\n", isdir ? 'd' : ' ', fullfilename);
				}
				g_free  (n);
			}

			g_free (fullfilename);
			// No need to free filename
		}

		g_dir_close (dir);
	}

	g_hash_table_remove_all (renamedFiles);		// All fullto's are freed here

	return (ret);
}

int main (int argc, char *argv[]) {
	int ret;
	char *pattern, *replacement;
	GOptionContext *context;
	GRegex *rx;
	GError *error;

	GOptionEntry entries[] = {
		{"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},
		{"apply", 'a', 0, G_OPTION_ARG_NONE, &apply, "Apply changes, do not simulate only", NULL},
		{"apply-without-overwrite-only", 'A', 0, G_OPTION_ARG_NONE, &apply_no_overwrite, "Simulate first, and then apply changes if no overwriting occurs", NULL},
		{"case-insensitive", 'i', 0, G_OPTION_ARG_NONE, &caseless, "Match pattern case-insensitively", NULL},
		{"recurse", 'r', 0, G_OPTION_ARG_NONE, &recurse, "Recurse into subdirectories", NULL},
		{"force-overwrite", 'f', 0, G_OPTION_ARG_NONE, &overwrite, "Overwrite existing files/directories", NULL},
		{"rename-directories", 'n', 0, G_OPTION_ARG_NONE, &rendirs, "Rename directories as well, not only files", NULL},
		{"match-extensions", 'e', 0, G_OPTION_ARG_NONE, &matchext, "Match pattern in file extensions as well", NULL},
		{"base-directory", 'd', 0, G_OPTION_ARG_FILENAME, &path, "Match files in path instead of current directory", "DIRECTORY"},
		{NULL}
	};

	gchar *addendum = \
		"Output Key:\n"
		"   # File/Directory not matching pattern\n"
		"   - File/Directory renamed (old name)\n"
		"   + File/Directory new name\n"
		"   e File/Directory new name, rename failed\n"
		"   ! File/Directory new name, overwrite disabled\n"
		"   * File/Directory new name, overwrite successful\n"
		"   E File/Directory new name, overwrite failed\n"
		"   $ File/Directory new name, would overwrite already-renamed file\n"
		"  ds Directory, not recursing\n"
		"  dr Directory, recursing\n"
		"An initial 'd' means 'directory'. # and dr are only shown in verbose mode.";

	context = g_option_context_new ("PATTERN REPLACEMENT - Command-line Batch File Renamer");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_set_description (context, addendum);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("Option parsing failed: %s\n", error -> message);
		g_error_free (error);

		gchar *help = g_option_context_get_help (context, TRUE, NULL);
		g_printerr ("%s", help);
		g_free (help);

		ret = 1;
	} else if (argc != 3) {
		gchar *help = g_option_context_get_help (context, TRUE, NULL);
		g_printerr ("%s", help);
		g_free (help);

		ret = 2;
	} else {
		pattern = argv[1];
		replacement = argv[2];

		error = NULL;
		GRegexCompileFlags cflags = G_REGEX_OPTIMIZE;
		if (caseless)
			cflags |= G_REGEX_CASELESS;
		if (!(rx = g_regex_new (pattern, cflags, 0, &error))) {
			g_critical ("Malformed pattern: %s", error -> message);
			g_error_free (error);
		} else {
			if (apply_no_overwrite) {
				// Force simulation
				apply = FALSE;
			}

			if (apply)
				g_print ("Applying changes\n");

			if (!path)
				path = g_strdup (".");

			gboolean no_overwrites = process_dir (path, rx, replacement);

			if (apply_no_overwrite) {
				if (no_overwrites) {
					g_print ("Simulation showed no overwrites, applying changes\n");
					apply = TRUE;
					process_dir (path, rx, replacement);
				} else {
					g_print ("Renaming would overwrite some files, changes not applied\n");
				}
			}

			g_regex_unref (rx);
		}

		ret = 0;
	}

	g_free (path);
	g_option_context_free (context);

	return (ret);
}
