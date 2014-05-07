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
static gboolean matchext = FALSE;


void process_dir (gchar *path, const GRegex *rx, const gchar *replacement) {
	GDir *dir;
	GError *error;
	GHashTable *renamedFiles;
	
	g_debug ("Processing directory: \"%s\"", path);
	
	renamedFiles = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	
	error = NULL;
	if (!(dir = g_dir_open (path, 0, &error))) {
		g_printerr ("Cannot get directory listing for \"%s\": %s", path, error -> message);
		g_error_free (error);
	} else {
		const gchar *filename;
		while ((filename = g_dir_read_name (dir))) {
			// '.' and '..' entries are already omitted by the great glib
			char *fullfilename = g_build_filename (path, filename, NULL);
			
			if (g_file_test (fullfilename, G_FILE_TEST_IS_DIR)) {
				// Directory, recurse only if requested
				if (recurse) {
					if (verbose)
						g_print ("dr %s\n", fullfilename);
					process_dir (fullfilename, rx, replacement);
				} else {
					g_print ("ds %s\n", fullfilename);
				}
				
				// After possibly recurring, rename if requested
				if (rendirs) {
					error = NULL;
					gchar *n = g_regex_replace (rx, filename, -1, 0, replacement, 0, &error);
					if (strcmp (filename, n) != 0) {
						// Regexp matched, try to rename
						g_print ("d- %s\n", fullfilename);
						
						gchar *fullto = g_build_filename (path, n, NULL);
						if (g_file_test (fullto, G_FILE_TEST_EXISTS)) {
							// Dir/File already exists
							if (overwrite && apply && g_rename (fullfilename, fullto) {
								g_print ("d* %s\n", fullto);
								g_hash_table_add (renamedFiles, fullto);
							} else {
								g_print ("d! %s\n", fullto);
							}
						} else if (g_hash_table_contains (renamedFiles, fullto)) {
							// File would overwrite an already-renamed file.
							// In this case we never overwrite.
							g_print (" $ %s\n", fullto);
							g_free  (fullto);
						} else {
							if (apply && g_rename (fullfilename, fullto) < 0)
								g_print ("de %s\n", fullto);
							else
								g_print ("d+ %s\n", fullto);
						}
						g_free  (fullto);
					} else {
						if (verbose)
							g_print ("d# %s\n", fullfilename);
					}
					g_free  (n);
				}
			} else {
				// File, always try to match & rename
				
				// Prepare to skip file extensions, in case
				gssize len = strlen (filename);
				gchar *lastdot = NULL;
				if (!matchext) {
					lastdot = strrchr (filename, '.');
					if (lastdot) {
						len = lastdot - filename;
					}
				}
				
				error = NULL;
				gchar *n = g_regex_replace (rx, filename, len, 0, replacement, 0, &error);
				if (strncmp (filename, n, (size_t) len) != 0) {
					// Regexp matched, try to rename
					g_print (" - %s\n", fullfilename);
					
					gchar *fullto;
					if (lastdot) {
						gchar *tmp = g_strconcat (n, lastdot, NULL);
						fullto = g_build_filename (path, tmp, NULL);
						g_free (tmp);
					} else {
						fullto = g_build_filename (path, n, NULL);
					}
					if (g_file_test (fullto, G_FILE_TEST_EXISTS)) {
						// File already exists
						if (overwrite && apply && g_rename (fullfilename, fullto) < 0) {
							g_hash_table_add (renamedFiles, fullto);
							g_print (" * %s\n", fullto);
						} else {
							g_print (" ! %s\n", fullto);
							g_free  (fullto);
						}
					} else if (g_hash_table_contains (renamedFiles, fullto)) {
						// File would overwrite an already-renamed file.
						// In this case we never overwrite.
						g_print (" $ %s\n", fullto);
						g_free  (fullto);
					} else {
						if (apply && g_rename (fullfilename, fullto) < 0) {
							g_print (" e %s\n", fullto);
							g_free  (fullto);
						} else {
							g_hash_table_add (renamedFiles, fullto);
							g_print (" + %s\n", fullto);
						}
					}
				} else {
					if (verbose)
						g_print (" # %s\n", fullfilename);
				}
				g_free  (n);
			}
			
			g_free (fullfilename);
			// No need to free filename
		}
		
		g_dir_close (dir);
	}
	
	g_hash_table_remove_all (renamedFiles);		// All fullto's are freed here
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
		"   * File/Directory new name, overwrite forced\n"
		"   ! File/Directory new name, overwrite disabled\n"
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
			if (apply)
				g_print ("Applying changes\n");
				
			if (!path)
				path = g_strdup (".");
			
			process_dir (path, rx, replacement);
			
			g_regex_unref (rx);
		}
		
		ret = 0;
	}
	
	g_free (path);
	g_option_context_free (context);
	
	return (ret);
}
