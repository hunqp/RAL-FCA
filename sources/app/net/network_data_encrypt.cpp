#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#include "app_dbg.h"
#include "rncryptor_c.h"
#include "network_data_encrypt.h"


static unsigned char* OpenSSL_Base64Decoder(const char *cipher, int *decodedLen) {
	BIO *bio, *b64;
    int cipherLen = strlen(cipher);
    unsigned char *buffers = (unsigned char *)malloc(cipherLen);
	if (!buffers) {
		return NULL;
	}
    memset(buffers, 0, cipherLen);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf((void *)cipher, -1);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); /* NO_WRAP (no newlines) */
    *decodedLen = BIO_read(bio, buffers, cipherLen);
    BIO_free_all(bio);
    return buffers;
}

std::string decryptMobileIncomeMessage(std::string &encrypt) {
    int decoder64Len = 0;
    unsigned char *decoder64 = OpenSSL_Base64Decoder(encrypt.c_str(), &decoder64Len);
    if (!decoder64) {
        APP_ERR("HOSTAPD - Invalid Decode Base64\r\n");
        return "";
    }
    int DecryptLen = 0;
    char errBuffers[512] = {0};
    unsigned char *Decrypt = rncryptorc_decrypt_data_with_password(
        (const unsigned char*)decoder64, decoder64Len,
        100,
        "05hQGF7bpdfakooDoXM6Ad632YE3yDZv", 
        strlen("05hQGF7bpdfakooDoXM6Ad632YE3yDZv"),
        &DecryptLen,
        errBuffers, sizeof(errBuffers));
    if (!Decrypt) {
        APP_ERR("HOSTAPD - Can't decrypt by RNCrypto\r\n");
        return "";
    }
    free(decoder64);
    APP_DBG("Decrypt -> %s\r\n", Decrypt);
    return std::string((char*)Decrypt, DecryptLen);
}