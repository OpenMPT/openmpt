/*

Copyright (c) 2011, 2012, Simon Howard

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted, provided
that the above copyright notice and this permission notice appear
in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib/lha_arch.h"

#include "extract.h"
#include "safe.h"

// Maximum number of dots in progress output:

#define MAX_PROGRESS_LEN 58

typedef struct {
	int invoked;
	LHAFileHeader *header;
	LHAOptions *options;
	char *filename;
	char *operation;
} ProgressCallbackData;

// Given a file header structure, get the path to extract to.
// Returns a newly allocated string that must be free()d.

static char *file_full_path(LHAFileHeader *header, LHAOptions *options)
{
	size_t len;
	char *result;
	char *p;

	// Full path, or filename only?

	len = 0;

	if (options->use_path && header->path != NULL) {
		len += strlen(header->path);
	}

	if (header->filename != NULL) {
		len += strlen(header->filename);
	}

	result = malloc(len + 1);

	if (result == NULL) {
		// TODO?
		exit(-1);
	}

	result[0] = '\0';

	if (options->use_path && header->path != NULL) {
		strcat(result, header->path);
	}

	if (header->filename != NULL) {
		strcat(result, header->filename);
	}

	// If the header contains leading '/' characters, it is an
	// absolute path (eg. /etc/passwd). Strip these leading
	// characters to always generate a relative path, otherwise
	// it can be a security risk - extracting an archive might
	// overwrite arbitrary files on the filesystem.

	p = result;

	while (*p == '/') {
		++p;
	}

	if (p > result) {
		memmove(result, p, strlen(p) + 1);
	}

	return result;
}

static void print_filename(char *filename, char *status)
{
	printf("\r");
	safe_printf("%s", filename);
	printf("\t- %s  ", status);
}

static void print_filename_brief(char *filename)
{
	printf("\r");
	safe_printf("%s :", filename);
}

// Callback function invoked during decompression progress.

static void progress_callback(unsigned int block,
                              unsigned int num_blocks,
                              void *data)
{
	ProgressCallbackData *progress = data;
	unsigned int factor;
	unsigned int i;

	progress->invoked = 1;

	// If the quiet mode options are specified, print a limited amount
	// of information without a progress bar (level 1) or no message
	// at all (level 2).

	if (progress->options->quiet >= 2) {
		return;
	} else if (progress->options->quiet == 1) {
		if (block == 0) {
			print_filename_brief(progress->filename);
			fflush(stdout);
		}

		return;
	}

	// Scale factor for blocks, so that the line is never too long.  When
	// MAX_PROGRESS_LEN is exceeded, the length is halved (factor=2), then
	// progressively larger scale factors are applied.

	factor = 1 + (num_blocks / MAX_PROGRESS_LEN);
	num_blocks = (num_blocks + factor - 1) / factor;

	// First call to specify number of blocks?

	if (block == 0) {
		print_filename(progress->filename, progress->operation);

		for (i = 0; i < num_blocks; ++i) {
			printf(".");
		}

		print_filename(progress->filename, progress->operation);
	} else if (((block + factor - 1) % factor) == 0) {
		// Otherwise, signal progress:

		printf("o");
	}

	fflush(stdout);
}

// Perform CRC check of an archived file.

static int test_archived_file_crc(LHAReader *reader,
                                  LHAFileHeader *header,
                                  LHAOptions *options)
{
	ProgressCallbackData progress;
	char *filename;
	int success;

	filename = file_full_path(header, options);

	if (options->dry_run) {
		if (strcmp(header->compress_method,
		           LHA_COMPRESS_TYPE_DIR) != 0) {
			safe_printf("VERIFY %s", filename);
			printf("\n");
		}

		free(filename);
		return 1;
	}

	progress.invoked = 0;
	progress.operation = "Testing  :";
	progress.options = options;
	progress.filename = filename;
	progress.header = header;

	success = lha_reader_check(reader, progress_callback, &progress);

	if (progress.invoked && options->quiet < 2) {
		if (success) {
			print_filename(filename, "Tested");
			printf("\n");
		} else {
			print_filename(filename, "CRC error");
			printf("\n");
		}

		fflush(stdout);
	}

	if (!success) {
		// TODO: Exit with error
	}

	free(filename);

	return success;
}

// Check that the specified directory exists, and create it if it
// does not.

static int check_parent_directory(char *path)
{
	LHAFileType file_type;

	file_type = lha_arch_exists(path);

	switch (file_type) {

		case LHA_FILE_DIRECTORY:
			// Already exists.
			break;

		case LHA_FILE_NONE:
			// Create the missing directory:

			if (!lha_arch_mkdir(path, 0755)) {
				fprintf(stderr,
				        "Failed to create parent directory %s\n",
				        path);
				return 0;
			}
			break;

		case LHA_FILE_FILE:
			fprintf(stderr, "Parent path %s is not a directory!\n",
			        path);
			return 0;

		case LHA_FILE_ERROR:
			fprintf(stderr, "Failed to stat %s\n", path);
			return 0;
	}

	return 1;
}

// Given a file header, create its parent directories as necessary.
// If include_final is zero, the final directory in the path is not
// created.

static int make_parent_directories(char *orig_path, int include_final)
{
	int result;
	char *p;
	char *path;
	char saved = 0;

	result = 1;
	path = strdup(orig_path);

	// Iterate through the string, finding each path separator. At
	// each place, temporarily chop off the end of the path to get
	// each parent directory in turn.

	p = path;

	do {
		p = strchr(p, '/');

		// Terminate string after the path separator.

		if (p != NULL) {
			saved = *(p + 1);
			*(p + 1) = '\0';
		}

		// Check if this parent directory exists and create it;
		// however, don't create the final directory in the path
		// if include_final is false. This is used for handling
		// directories, where we should only create missing
		// parent directories - not the directory itself.
		// eg. for:
		//
		//  -lhd-  subdir/subdir2/
		//
		// Only create subdir/ - subdir/subdir2/ is created
		// by the call to lha_reader_extract.

		if (include_final || strcmp(orig_path, path) != 0) {
			if (!check_parent_directory(path)) {
				result = 0;
				break;
			}
		}

		// Restore character that was after the path separator
		// and advance to the next path.

		if (p != NULL) {
			*(p + 1) = saved;
			++p;
		}
	} while (p != NULL);

	free(path);

	return result;
}

// Prompt the user with a message, and return the first character of
// the typed response.

static char prompt_user(char *message)
{
	char result;
	int c;

	fprintf(stderr, "%s", message);
	fflush(stderr);

	// Read characters until a newline is found, saving the first
	// character entered.

	result = 0;

	do {
		c = getchar();

		if (c < 0) {
			exit(-1);
		}

		if (result == 0) {
			result = c;
		}
	} while (c != '\n');

	return result;
}

// A file to be extracted already exists. Apply the overwrite policy
// to decide whether to overwrite the existing file, prompting the
// user if necessary.

static int confirm_file_overwrite(char *filename, LHAOptions *options)
{
	char response;

	switch (options->overwrite_policy) {
		case LHA_OVERWRITE_PROMPT:
			break;
		case LHA_OVERWRITE_SKIP:
			return 0;
		case LHA_OVERWRITE_ALL:
			return 1;
	}

	for (;;) {
		safe_fprintf(stderr, "%s ", filename);

		response = prompt_user("OverWrite ?(Yes/[No]/All/Skip) ");

		switch (tolower((unsigned int) response)) {
			case 'y':
				return 1;
			case 'n':
			case '\n':
				return 0;
			case 'a':
				options->overwrite_policy = LHA_OVERWRITE_ALL;
				return 1;
			case 's':
				options->overwrite_policy = LHA_OVERWRITE_SKIP;
				return 0;
			default:
				break;
		}
	}

	return 0;
}

// Check if the file pointed to by the specified header exists.

static int file_exists(char *filename)
{
	LHAFileType file_type;

	file_type = lha_arch_exists(filename);

	if (file_type == LHA_FILE_ERROR) {
		fprintf(stderr, "Failed to read file type of '%s'\n", filename);
		exit(-1);
	}

	return file_type != LHA_FILE_NONE;
}

// Extract an archived file.

static int extract_archived_file(LHAReader *reader,
                                 LHAFileHeader *header,
                                 LHAOptions *options)
{
	ProgressCallbackData progress;
	char *path;
	char *filename;
	int success;
	int is_dir, is_symlink;

	filename = file_full_path(header, options);
	is_symlink = header->symlink_target != NULL;
	is_dir = !strcmp(header->compress_method, LHA_COMPRESS_TYPE_DIR)
	      && !is_symlink;

	// Print appropriate message and stop if we are performing a dry run.
	// The message if we have an existing file is weird, but this is just
	// accurately duplicating what the Unix LHA tool says.
	// The symlink handling is particularly odd - they are treated as
	// directories (a bleed-through of the way in which symlinks are
	// stored).

	if (options->dry_run) {
		if (is_dir) {
			safe_printf("EXTRACT %s (directory)", filename);
		} else if (header->symlink_target != NULL) {
			safe_printf("EXTRACT %s|%s (directory)",
			            filename, header->symlink_target);
		} else if (file_exists(filename)) {
			safe_printf("EXTRACT %s but file is exist.", filename);
		} else {
			safe_printf("EXTRACT %s", filename);
		}
		printf("\n");

		free(filename);
		return 1;
	}

	// If a file already exists with this name, confirm overwrite.

	if (!is_dir && !is_symlink && file_exists(filename)
	 && !confirm_file_overwrite(filename, options)) {
		if (options->overwrite_policy == LHA_OVERWRITE_SKIP) {
			safe_printf("%s : Skipped...", filename);
			printf("\n");
		}
		free(filename);
		return 1;
	}

	// No need to extract directories if use_path is disabled.

	if (!options->use_path && is_dir) {
		return 1;
	}

	// Create parent directories for file:

	if (options->use_path && header->path != NULL) {
		path = header->path;

		while (path[0] == '/') {
			++path;
		}

		if (*path != '\0' && !make_parent_directories(path, !is_dir)) {
			free(filename);
			return 0;
		}
	}

	progress.invoked = 0;
	progress.operation = "Melting  :";
	progress.options = options;
	progress.header = header;
	progress.filename = filename;

	success = lha_reader_extract(reader, filename,
	                             progress_callback, &progress);

	if (!lha_reader_current_is_fake(reader) && options->quiet < 2) {
		if (progress.invoked) {
			if (success) {
				print_filename(filename, "Melted");
				printf("\n");
			} else {
				print_filename(filename, "Failure");
				printf("\n");
			}
		} else if (is_symlink) {
			safe_printf("Symbolic Link %s -> %s", filename,
			            header->symlink_target);
			printf("\n");
		}

		fflush(stdout);
	}

	if (!success) {
		// TODO: Exit with error
	}

	free(filename);

	return success;
}

// lha -t command.

int test_file_crc(LHAFilter *filter, LHAOptions *options)
{
	int result;

	result = 1;

	for (;;) {
		LHAFileHeader *header;

		header = lha_filter_next_file(filter);

		if (header == NULL) {
			break;
		}

		if (!test_archived_file_crc(filter->reader, header, options)) {
			result = 0;
		}
	}

	return result;
}

// lha -e / -x

int extract_archive(LHAFilter *filter, LHAOptions *options)
{
	int result;

	// Change directory before extract? (-w option).

	if (options->extract_path != NULL) {
		if (!make_parent_directories(options->extract_path, 1)) {
			fprintf(stderr,
			        "Failed to create extract directory: %s.\n",
			        options->extract_path);
			exit(-1);
		}

		if (!lha_arch_chdir(options->extract_path)) {
			fprintf(stderr, "Failed to change directory to %s.\n",
			        options->extract_path);
			exit(-1);
		}
	}

	result = 1;

	for (;;) {
		LHAFileHeader *header;

		header = lha_filter_next_file(filter);

		if (header == NULL) {
			break;
		}

		if (!extract_archived_file(filter->reader, header, options)) {
			result = 0;
		}
	}

	return result;
}

