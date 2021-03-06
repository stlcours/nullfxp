/* info.c --- 
 * 
 * Author: liuguangzhao
 * Copyright (C) 2007-2010 liuguangzhao@users.sf.net
 * URL: http://www.qtchina.net http://nullget.sourceforge.net
 * Created: 2009-07-19 10:56:03 +0000
 * Version: $Id$
 */

#include <assert.h>

#include "libssh2_priv.h"
#include "mac.h"
#include "sftp.h"

#include "ssh_info.h"

LIBSSH2_API 
int libssh2_sftp_get_version(LIBSSH2_SFTP * sftp)
{
    return sftp->version;	
    return 0;
}

// add by liuguangzhao@users.sf.net
LIBSSH2_API char * libssh2_session_get_remote_version(LIBSSH2_SESSION *session)
{
	return session->banner_TxRx_banner;
}

LIBSSH2_API char ** libssh2_session_get_remote_info(LIBSSH2_SESSION *session, char ** info_vec)
{
	char *info_buff;
	const LIBSSH2_KEX_METHOD *kex;
	const LIBSSH2_HOSTKEY_METHOD *hostkey;
	libssh2_endpoint_data *remote, *local;
	// char **info_vec = calloc(10, sizeof(char*));
    char fingerprint[50], *fprint = fingerprint;
    int i;

	// memset(info_vec, 0, 10 * sizeof(char*));
		
	kex = session->kex;
	hostkey = session->hostkey;
	remote = &session->remote;
	local = &session->local;
	
    for(i = 0; i < 16; i++, fprint += 3) {
        snprintf(fprint, 4, "%02x:", session->server_hostkey_md5[i]);
    }
    *(--fprint) = '\0';
	
	info_buff = malloc(512);
	memset(info_buff,0,512);
	snprintf(info_buff,510, 
             "SSH Server: %s\n"
             "Key exchange method: %s\n"
             "Key fingerprint: %s\n"
             "Host key method: %s , Hash length: %ld\n"
             "S->C Crypt name: %s, secret length: %d\n"
             "S->C Mac name: %s, Key length: %d\n"
             "C->S Crypt name: %s, secret length: %d\n"
             "C->S Mac name: %s, Key length: %d\n"
             "Comp name: %s\n"
             , remote->banner, kex->name,
             fingerprint,
             hostkey->name, hostkey->hash_len,
             remote->crypt->name, remote->crypt->secret_len,
             remote->mac->name, remote->mac->key_len,
             local->crypt->name, local->crypt->secret_len,
             local->mac->name, local->mac->key_len,
             remote->comp->name);
 
	strcpy(info_vec[0], info_buff);
	strcpy(info_vec[1], kex->name);
	strcpy(info_vec[2], hostkey->name);
	strcpy(info_vec[3], fingerprint);
	strcpy(info_vec[4], local->crypt->name);
    strcpy(info_vec[5], local->mac->name);
	strcpy(info_vec[6], remote->crypt->name);
    strcpy(info_vec[7], remote->mac->name);

	free(info_buff);
	
	return info_vec;
}

LIBSSH2_API LIBSSH2_SESSION *libssh2_session_for_sftp(LIBSSH2_SFTP *sftp)
{
    LIBSSH2_SESSION *sess = NULL;
    LIBSSH2_CHANNEL *chan = NULL;

    if (sftp == NULL) {
    } else {
        chan = sftp->channel;
        if (chan == NULL) {
        } else {
            sess = chan->session;
        }
    }

    return sess;
}

LIBSSH2_API int libssh2_publickey_is_privatekey(const char *keypath, const char *passphrase)
{
    int       st;
    BIO*      bp;
    EVP_PKEY* pk;

    bp = BIO_new_file(keypath, "r");
    if (bp == NULL) {
        return -1;
    }
    if (!EVP_get_cipherbyname("des")) {
        /* If this cipher isn't loaded it's a pretty good indication that none
         * are.  I have *NO DOUBT* that there's a better way to deal with this
         * ($#&%#$(%$#( Someone buy me an OpenSSL manual and I'll read up on
         * it.
         */
        OpenSSL_add_all_ciphers();
    }
    BIO_reset(bp);
    pk = PEM_read_bio_PrivateKey(bp, NULL, NULL, (void*)passphrase);
    BIO_free(bp);

    if (pk == NULL) {
        /* _libssh2_error(session, */
        /*                LIBSSH2_ERROR_FILE, */
        /*                "Wrong passphrase or invalid/unrecognized " */
        /*                "private key file format"); */
        return -1;
    }

    return 1;
}
