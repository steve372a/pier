/* curl_stubs.c - stubs for optional curl features not built in this configuration
   (HTTP/3, SSH, missing OpenSSL 1.1+ APIs, MinGW ABI bridge).
   Never actually called by pier-get. */

#include <stddef.h>

/* --- ngtcp2 (HTTP/3 QUIC) stubs --- */

void ngtcp2_path_storage_zero(void *p) { (void)p; }
int ngtcp2_conn_get_handshake_completed(void *c) { (void)c; return 1; }
int ngtcp2_settings_default_versioned(int v, void *s, size_t sz) { (void)v; (void)s; (void)sz; return 0; }
int ngtcp2_transport_params_default_versioned(int v, void *p, size_t sz) { (void)v; (void)p; (void)sz; return 0; }
void ngtcp2_addr_init(void *a, void *f, void *addr, size_t alen, int port) { (void)a; (void)f; (void)addr; (void)alen; (void)port; }
int ngtcp2_conn_client_new_versioned(void *c, void *h, void *d, void *m, int v, void *s, size_t sz) { (void)c; (void)h; (void)d; (void)m; (void)v; (void)s; (void)sz; return -1; }
void *ngtcp2_crypto_ossl_ctx_new(void *ssl) { (void)ssl; return NULL; }
int ngtcp2_conn_set_tls_native_handle(void *c, void *ssl) { (void)c; (void)ssl; return -1; }
int ngtcp2_crypto_ossl_configure_client_session(void *s) { (void)s; return -1; }
void ngtcp2_ccerr_default(void *e) { (void)e; }
int ngtcp2_conn_in_draining_period(void *c) { (void)c; return 0; }
int ngtcp2_conn_get_ccerr(void *c, void *e) { (void)c; (void)e; return -1; }

int ngtcp2_crypto_client_initial_cb(void *c, void *d, void *u) { (void)c; (void)d; (void)u; return -1; }
int ngtcp2_crypto_recv_crypto_data_cb(void *c, int l, void *d, int dl, void *u) { (void)c; (void)l; (void)d; (void)dl; (void)u; return -1; }
int ngtcp2_crypto_encrypt_cb(void *d, void *p, void *o, void *u) { (void)d; (void)p; (void)o; (void)u; return -1; }
int ngtcp2_crypto_decrypt_cb(void *d, void *p, void *o, void *u) { (void)d; (void)p; (void)o; (void)u; return -1; }
int ngtcp2_crypto_hp_mask_cb(void *d, void *p, void *o, void *u) { (void)d; (void)p; (void)o; (void)u; return -1; }
int ngtcp2_crypto_recv_retry_cb(void *c, void *d, void *u) { (void)c; (void)d; (void)u; return -1; }
int ngtcp2_crypto_update_key_cb(void *d, void *p, void *o, void *u) { (void)d; (void)p; (void)o; (void)u; return -1; }
int ngtcp2_crypto_delete_crypto_aead_ctx_cb(void *c, void *d, void *u) { (void)c; (void)d; (void)u; return 0; }
int ngtcp2_crypto_delete_crypto_cipher_ctx_cb(void *c, void *d, void *u) { (void)c; (void)d; (void)u; return 0; }
int ngtcp2_crypto_get_path_challenge_data_cb(void *c, void *d, void *u) { (void)c; (void)d; (void)u; return -1; }
int ngtcp2_crypto_get_path_challenge_data2_cb(void *c, void *d, void *u) { (void)c; (void)d; (void)u; return -1; }

/* --- nghttp3 (HTTP/3) stubs (may appear as missing) --- */

int nghttp3_conn_client_new(void *c, void *d, void *m) { (void)c; (void)d; (void)m; return -1; }
void nghttp3_conn_del(void *c) { (void)c; }
int nghttp3_conn_bind_control_stream(void *c, void *s) { (void)c; (void)s; return -1; }
int nghttp3_conn_bind_qpack_streams(void *c, void *e, void *d) { (void)c; (void)e; (void)d; return -1; }
int nghttp3_conn_submit_request(void *c, void *h, void *d, void *u) { (void)c; (void)h; (void)d; (void)u; return -1; }
int nghttp3_conn_read_stream(void *c, int sid, void *d, size_t dl, int f) { (void)c; (void)sid; (void)d; (void)dl; (void)f; return -1; }
int nghttp3_conn_writev_stream(void *c, int sid, int *f, void *v, int c2) { (void)c; (void)sid; (void)f; (void)v; (void)c2; return -1; }
int nghttp3_conn_shutdown_stream_read(void *c, int sid) { (void)c; (void)sid; return -1; }
int nghttp3_conn_shutdown_stream_write(void *c, int sid) { (void)c; (void)sid; return -1; }
int nghttp3_conn_block_stream(void *c, int sid) { (void)c; (void)sid; return -1; }
int nghttp3_conn_shutdown_stream(void *c, int sid) { (void)c; (void)sid; return -1; }
long long nghttp3_conn_get_num_blocked_streams(void *c) { (void)c; return 0; }
void nghttp3_conn_set_max_client_request_body_size(void *c, long long v) { (void)c; (void)v; }
int nghttp3_strerror(int e, void *b, size_t blen) { (void)e; (void)b; (void)blen; return 0; }

/* --- OpenSSL 1.1+ stubs (missing in this OpenSSL version) --- */

const char *OpenSSL_version(int type) { (void)type; return "OpenSSL"; }
int BIO_get_shutdown(void *b) { (void)b; return 0; }

/* --- libssh2 stub --- */

void libssh2_exit(void) {}

/* --- MinGW ABI bridge: old MinGW -> MinGW-w64 --- */

int __sys_nerr_impl;
int *_imp____sys_nerr = &__sys_nerr_impl;

struct _stat64i32;

int __cdecl _imp___stat32i64(const char *path, void *buf) {
    extern int __cdecl _stat64i32(const char *, void *);
    return _stat64i32(path, buf);
}

int __cdecl _imp___fstat32i64(int fd, void *buf) {
    extern int __cdecl _fstat64i32(int, void *);
    return _fstat64i32(fd, buf);
}

unsigned long long __stdcall GetTickCount64(void) {
    extern unsigned int __stdcall timeGetTime(void);
    return (unsigned long long)timeGetTime();
}