/* $OpenBSD: sftp-client.h,v 1.14 2005/04/26 12:59:02 jmc Exp $ */

/*
 * Copyright (c) 2001-2004 Damien Miller <djm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/* Client side of SSH2 filexfer protocol */

#ifndef _SFTP_CLIENT_H
#define _SFTP_CLIENT_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "sftp-common.h"

                 
#define MIN_READ_SIZE       512
                 
                 
typedef struct SFTP_DIRENT SFTP_DIRENT;

struct SFTP_DIRENT {
	char *filename;
	char *longname;
	Attrib a;
};

struct sftp_conn {
    int fd_in;
    int fd_out;
    u_int transfer_buflen;
    u_int num_requests;
    u_int version;
    u_int msg_id;
};

/*
 * Initialise a SSH filexfer connection. Returns NULL on error or
 * a pointer to a initialized sftp_conn struct on success.
 */
struct sftp_conn *do_init(int, int, u_int, u_int);

u_int sftp_proto_version(struct sftp_conn *);

/* Close file referred to by 'handle' */
int do_close(struct sftp_conn *, char *, u_int);

/* Read contents of 'path' to NULL-terminated array 'dir' */
int do_readdir(struct sftp_conn *, char *, SFTP_DIRENT ***);

/* Frees a NULL-terminated array of SFTP_DIRENTs (eg. from do_readdir) */
void free_sftp_dirents(SFTP_DIRENT **);

/* Delete file 'path' */
int do_rm(struct sftp_conn *, char *);

/* Create directory 'path' */
int do_mkdir(struct sftp_conn *, char *, Attrib *);

/* Remove directory 'path' */
int do_rmdir(struct sftp_conn *, char *);

/* Get file attributes of 'path' (follows symlinks) */
Attrib *do_stat(struct sftp_conn *, char *, int);

/* Get file attributes of 'path' (does not follow symlinks) */
Attrib *do_lstat(struct sftp_conn *, char *, int);

/* Get file attributes of open file 'handle' */
Attrib *do_fstat(struct sftp_conn *, char *, u_int, int);

/* Set file attributes of 'path' */
int do_setstat(struct sftp_conn *, char *, Attrib *);

/* Set file attributes of open file 'handle' */
int do_fsetstat(struct sftp_conn *, char *, u_int, Attrib *);

/* Canonicalise 'path' - caller must free result */
char *do_realpath(struct sftp_conn *, char *);

/* Rename 'oldpath' to 'newpath' */
int do_rename(struct sftp_conn *, char *, char *);

/* Rename 'oldpath' to 'newpath' */
int do_symlink(struct sftp_conn *, char *, char *);

/* Return target of symlink 'path' - caller must free result */
char *do_readlink(struct sftp_conn *, char *);

/* XXX: add callbacks to do_download/do_upload so we can do progress meter */

/*
 * Download 'remote_path' to 'local_path'. Preserve permissions and times
 * if 'pflag' is set
 */
int do_download(struct sftp_conn *, char *, char *, int);

/*
 * Upload 'local_path' to 'remote_path'. Preserve permissions and times
 * if 'pflag' is set
 */
int do_upload(struct sftp_conn *, char *, char *, int);

//the static 函数，但是现在需要在其他地方调用的
void      send_msg(int fd, Buffer *m);
void
        get_msg(int fd, Buffer *m);

char *
        get_handle ( int fd, u_int expected_id, u_int *len );
void
        send_read_request ( int fd_out, u_int id, u_int64_t offset, u_int len,
                            char *handle, u_int handle_len );                

#ifdef __cplusplus
}
#endif
    
#endif
