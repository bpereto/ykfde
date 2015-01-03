/*
 * (C) 2014 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * compile with:
 * $ gcc -o mkcpio mkcpio.c -larchive
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#define _XOPEN_SOURCE 600

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>

#include "../config.h"

int add_dir(struct archive *archive, const char * path) {
	struct stat st;
	struct archive_entry *entry;
	int8_t rc;

	/* initialize struct stat for directories from root */
	if ((rc = stat("/", &st)) < 0) {
		perror("stat() failed");
		goto out;
	}

	if ((entry = archive_entry_new()) == NULL) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_entry_new() failed");
		goto out;
	}

	archive_entry_set_pathname(entry, path);
	archive_entry_set_filetype(entry, AE_IFDIR);
	archive_entry_copy_stat(entry, &st);
	if (archive_write_header(archive, entry) != ARCHIVE_OK) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_write_header() failed");
		goto out;
	}
	archive_entry_free(entry);

	rc = EXIT_SUCCESS;

out:
	return rc;
}

int main(int argc, const char **argv) {
	char cpiotmpfile[] = CPIOTMPFILE;
	struct archive *archive;
	struct archive_entry *entry;
	struct stat st;
	char buff[64];
	int len, fdfile, fdarchive;
	DIR * dir;
	struct dirent * ent;
	char * filename, * path;
	off_t pathlength = 0;
	int8_t rc = EXIT_FAILURE;

	if ((rc = fdarchive = mkstemp(cpiotmpfile)) < 0) {
		perror("mkstemp() failed");
		goto out10;
	}

	if ((archive = archive_write_new()) == NULL) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_write_new() failed.\n");
		goto out10;
	}

	if (archive_write_set_format_cpio_newc(archive) != ARCHIVE_OK) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_write_set_format_cpio_newc() failed.\n");
		goto out10;
	}

	if (archive_write_open_fd(archive, fdarchive) != ARCHIVE_OK) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_write_open_fd() failed.\n");
		goto out10;
	}

        while (1) {
		path = strdup(CHALLENGEDIR + 1);
		if (strstr(path + pathlength, "/") == NULL)
			break;
		*strstr(path + pathlength, "/") = 0;
		pathlength = strlen(path) + 1;

		if ((rc = add_dir(archive, path)) < 0) {
			fprintf(stderr, "add_dir() failed");
			goto out10;
		}

		free(path);
	}

	if ((dir = opendir(CHALLENGEDIR)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			filename = malloc(sizeof(CHALLENGEDIR) + strlen(ent->d_name) + 1);
			sprintf(filename, CHALLENGEDIR "%s", ent->d_name);

			if ((rc = stat(filename, &st)) < 0) {
				perror("stat() failed");
				goto out10;
			}

			if (S_ISREG(st.st_mode)) {
				if ((entry = archive_entry_new()) == NULL) {
					rc = EXIT_FAILURE;
					fprintf(stderr, "archive_entry_new() failed.\n");
					goto out10;
				}

				/* these do not return exit code */
				archive_entry_set_pathname(entry, filename + 1);
				archive_entry_set_size(entry, st.st_size);
				archive_entry_set_filetype(entry, AE_IFREG);
				archive_entry_set_perm(entry, 0644);

				if (archive_write_header(archive, entry) != ARCHIVE_OK) {
					rc = EXIT_FAILURE;
					fprintf(stderr, "archive_write_header() failed");
					goto out10;
				}

				if ((rc = fdfile = open(filename, O_RDONLY)) < 0) {
					perror("open() failed");
					goto out10;
				}

				if ((rc = len = read(fdfile, buff, sizeof(buff))) < 0) {
					perror("read() failed");
					goto out10;
				}

				while (len > 0) {
					if (( rc = archive_write_data(archive, buff, len)) < 0) {
						fprintf(stderr, "archive_write_data() failed");
						goto out10;
					}

					if ((rc = len = read(fdfile, buff, sizeof(buff))) < 0) {
						perror("read() failed");
						goto out10;
					}
				}

				if ((rc = close(fdfile)) < 0) {
					perror("close() failed");
					goto out10;
				}

				archive_entry_free(entry);
			}
			free(filename);
		}
		if ((rc = closedir(dir)) < 0) {
			perror("closedir() failed");
			goto out10;
		}
	} else {
		rc = EXIT_FAILURE;
		perror("opendir() failed");
		goto out10;
	}

	if (archive_write_close(archive) != ARCHIVE_OK) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_write_close() failed");
		goto out10;
	}

	if (archive_write_free(archive) != ARCHIVE_OK) {
		rc = EXIT_FAILURE;
		fprintf(stderr, "archive_write_free() failed");
		goto out10;
	}

	if (access(CPIOFILE, F_OK) == 0 && (rc = unlink(CPIOFILE)) < 0) {
		perror("unkink() failed");
		goto out10;
	}

	if ((rc = rename(cpiotmpfile, CPIOFILE)) < 0) {
		perror("rename() failed");
		goto out10;
	}

	rc = EXIT_SUCCESS;

out10:
	if (access(cpiotmpfile, F_OK) == 0)
		unlink(cpiotmpfile);

	return rc;
}

// vim: set syntax=c:
