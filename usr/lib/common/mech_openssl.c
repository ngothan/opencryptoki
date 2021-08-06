/*
 * COPYRIGHT (c) International Business Machines Corp. 2021
 *
 * This program is provided under the terms of the Common Public License,
 * version 1.0 (CPL-1.0). Any use, reproduction or distribution for this
 * software constitutes recipient's acceptance of CPL-1.0 terms which can be
 * found in the file LICENSE file or at
 * https://opensource.org/licenses/cpl1.0.php
 */


#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/opensslv.h>

#include "pkcs11types.h"
#include "defs.h"
#include "host_defs.h"
#include "h_extern.h"
#include "tok_spec_struct.h"
#include "trace.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#if OPENSSL_VERSION_PREREQ(3, 0)
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#endif

CK_RV openssl_specific_rsa_keygen(TEMPLATE *publ_tmpl, TEMPLATE *priv_tmpl)
{
    CK_ATTRIBUTE *publ_exp = NULL;
    CK_ATTRIBUTE *attr = NULL;
    CK_ULONG mod_bits;
    CK_BBOOL flag;
    CK_RV rc;
    CK_ULONG BNLength;
#if !OPENSSL_VERSION_PREREQ(3, 0)
    const RSA *rsa = NULL;
    const BIGNUM *bignum = NULL;
#else
    BIGNUM *bignum = NULL;
#endif
    CK_BYTE *ssl_ptr = NULL;
    BIGNUM *e = NULL;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;

    rc = template_attribute_get_ulong(publ_tmpl, CKA_MODULUS_BITS, &mod_bits);
    if (rc != CKR_OK) {
        TRACE_ERROR("%s\n", ock_err(ERR_TEMPLATE_INCOMPLETE));
        return CKR_TEMPLATE_INCOMPLETE; // should never happen
    }

    // we don't support less than 512 bit keys in the sw
    if (mod_bits < 512 || mod_bits > OPENSSL_RSA_MAX_MODULUS_BITS) {
        TRACE_ERROR("%s\n", ock_err(ERR_KEY_SIZE_RANGE));
        return CKR_KEY_SIZE_RANGE;
    }

    rc = template_attribute_get_non_empty(publ_tmpl, CKA_PUBLIC_EXPONENT,
                                          &publ_exp);
    if (rc != CKR_OK) {
        TRACE_ERROR("%s\n", ock_err(ERR_TEMPLATE_INCOMPLETE));
        return CKR_TEMPLATE_INCOMPLETE;
    }

    if (publ_exp->ulValueLen > sizeof(CK_ULONG)) {
        TRACE_ERROR("%s\n", ock_err(ERR_ATTRIBUTE_VALUE_INVALID));
        return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    e = BN_new();
    if (e == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        return CKR_HOST_MEMORY;
    }
    BN_bin2bn(publ_exp->pValue, publ_exp->ulValueLen, e);

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }

    if (EVP_PKEY_keygen_init(ctx) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
    if (mod_bits > INT_MAX
        || EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, mod_bits) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#if !OPENSSL_VERSION_PREREQ(3, 0)
    if (EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, e) != 1) {
#else
    if (EVP_PKEY_CTX_set1_rsa_keygen_pubexp(ctx, e) != 1) {
#endif
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#if !OPENSSL_VERSION_PREREQ(3, 0)
    e = NULL; // will be freed as part of the context
#endif
    if (EVP_PKEY_keygen(ctx, &pkey) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#if !OPENSSL_VERSION_PREREQ(3, 0)
    if ((rsa = EVP_PKEY_get0_RSA(pkey)) == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
    RSA_get0_key(rsa, &bignum, NULL, NULL);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_MODULUS, ssl_ptr, BNLength, &attr);    // in bytes
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(publ_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(attr);
        goto done;
    }
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // Public Exponent
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_key(rsa, NULL, &bignum, NULL);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_E, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    // in bytes
    rc = build_attribute(CKA_PUBLIC_EXPONENT, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(publ_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(attr);
        goto done;
    }

    /* add public exponent to the private template. Its already an attribute in
     * the private template at this point, we're just making its value correct
     */
    rc = build_attribute(CKA_PUBLIC_EXPONENT, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(attr);
        goto done;
    }
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // local = TRUE
    //
    flag = TRUE;
    rc = build_attribute(CKA_LOCAL, &flag, sizeof(CK_BBOOL), &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(publ_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(attr);
        goto done;
    }

    //
    // now, do the private key
    //
    // Cheat here and put the whole original key into the CKA_VALUE... remember
    // to force the system to not return this for RSA keys..

    // Add the modulus to the private key information
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_key(rsa, &bignum, NULL, NULL);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_MODULUS, ssl_ptr, BNLength, &attr);    // in bytes
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(attr);
        goto done;
    }
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // Private Exponent
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_key(rsa, NULL, NULL, &bignum);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_D, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_PRIVATE_EXPONENT, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        OPENSSL_cleanse(attr, sizeof(CK_ATTRIBUTE) + attr->ulValueLen);
        free(attr);
        goto done;
    }
    OPENSSL_cleanse(ssl_ptr, BNLength);
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // prime #1: p
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_factors(rsa, &bignum, NULL);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR1, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_PRIME_1, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        OPENSSL_cleanse(attr, sizeof(CK_ATTRIBUTE) + attr->ulValueLen);
        free(attr);
        goto done;
    }
    OPENSSL_cleanse(ssl_ptr, BNLength);
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // prime #2: q
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_factors(rsa, NULL, &bignum);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR2, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_PRIME_2, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        OPENSSL_cleanse(attr, sizeof(CK_ATTRIBUTE) + attr->ulValueLen);
        free(attr);
        goto done;
    }
    OPENSSL_cleanse(ssl_ptr, BNLength);
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // exponent 1: d mod(p-1)
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_crt_params(rsa, &bignum, NULL, NULL);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT1, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_EXPONENT_1, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        OPENSSL_cleanse(attr, sizeof(CK_ATTRIBUTE) + attr->ulValueLen);
        free(attr);
        goto done;
    }
    OPENSSL_cleanse(ssl_ptr, BNLength);
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // exponent 2: d mod(q-1)
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_crt_params(rsa, NULL, &bignum, NULL);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT2, &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_EXPONENT_2, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        OPENSSL_cleanse(attr, sizeof(CK_ATTRIBUTE) + attr->ulValueLen);
        free(attr);
        goto done;
    }
    OPENSSL_cleanse(ssl_ptr, BNLength);
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    // CRT coefficient:  q_inverse mod(p)
#if !OPENSSL_VERSION_PREREQ(3, 0)
    RSA_get0_crt_params(rsa, NULL, NULL, &bignum);
#else
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_COEFFICIENT1,
                               &bignum)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
#endif
    BNLength = BN_num_bytes(bignum);
    ssl_ptr = malloc(BNLength);
    if (ssl_ptr == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }
    BNLength = BN_bn2bin(bignum, ssl_ptr);
    rc = build_attribute(CKA_COEFFICIENT, ssl_ptr, BNLength, &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        OPENSSL_cleanse(attr, sizeof(CK_ATTRIBUTE) + attr->ulValueLen);
        free(attr);
        goto done;
    }
    OPENSSL_cleanse(ssl_ptr, BNLength);
    free(ssl_ptr);
    ssl_ptr = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    BN_free(bignum);
    bignum = NULL;
#endif

    flag = TRUE;
    rc = build_attribute(CKA_LOCAL, &flag, sizeof(CK_BBOOL), &attr);
    if (rc != CKR_OK) {
        TRACE_DEVEL("build_attribute failed\n");
        goto done;
    }
    rc = template_update_attribute(priv_tmpl, attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(attr);
        goto done;
    }

done:
    if (ssl_ptr != NULL) {
        OPENSSL_cleanse(ssl_ptr, BNLength);
        free(ssl_ptr);
    }
    if (pkey != NULL)
        EVP_PKEY_free(pkey);
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);
    if (e != NULL)
        BN_free(e);
#if OPENSSL_VERSION_PREREQ(3, 0)
    if (bignum != NULL)
        BN_free(bignum);
#endif
    return rc;
}

// convert from the local PKCS11 template representation to
// the underlying requirement
// returns the pointer to the local key representation
static EVP_PKEY *rsa_convert_public_key(OBJECT *key_obj)
{
    CK_BBOOL rc;
    CK_ATTRIBUTE *modulus = NULL;
    CK_ATTRIBUTE *pub_exp = NULL;
    EVP_PKEY *pkey = NULL;
    BIGNUM *bn_mod, *bn_exp;
#if OPENSSL_VERSION_PREREQ(3, 0)
    EVP_PKEY_CTX *pctx = NULL;
    OSSL_PARAM_BLD *tmpl = NULL;
    OSSL_PARAM *params = NULL;
#else
    RSA *rsa;
#endif

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &modulus);
    if (rc != CKR_OK)
        return NULL;
    rc = template_attribute_get_non_empty(key_obj->template,
                                          CKA_PUBLIC_EXPONENT, &pub_exp);
    if (rc != CKR_OK)
        return NULL;

    // Create and init BIGNUM structs
    bn_mod = BN_new();
    bn_exp = BN_new();

    if (bn_exp == NULL || bn_mod == NULL) {
        if (bn_mod)
            free(bn_mod);
        if (bn_exp)
            free(bn_exp);
        return NULL;
    }
    // Convert from strings to BIGNUMs
    BN_bin2bn((unsigned char *) modulus->pValue, modulus->ulValueLen, bn_mod);
    BN_bin2bn((unsigned char *) pub_exp->pValue, pub_exp->ulValueLen, bn_exp);

#if !OPENSSL_VERSION_PREREQ(3, 0)
    // Create an RSA key struct to return
    rsa = RSA_new();
    if (rsa == NULL) {
        if (bn_mod)
             free(bn_mod);
         if (bn_exp)
             free(bn_exp);
        return NULL;
    }

    RSA_set0_key(rsa, bn_mod, bn_exp, NULL);

    pkey = EVP_PKEY_new();
    if (pkey == NULL) {
       RSA_free(rsa);
       return NULL;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa) != 1) {
        RSA_free(rsa);
        EVP_PKEY_free(pkey);
        return NULL;
    }
#else
    tmpl = OSSL_PARAM_BLD_new();
    if (tmpl == NULL)
        goto out;

    if (!OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_N, bn_mod) ||
        !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_E, bn_exp))
        goto out;

    params = OSSL_PARAM_BLD_to_param(tmpl);
    if (params == NULL)
        goto out;

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (pctx == NULL)
        goto out;

    if (!EVP_PKEY_fromdata_init(pctx) ||
        !EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params))
        goto out;

out:
    if (pctx != NULL)
        EVP_PKEY_CTX_free(pctx);
    if (tmpl != NULL)
        OSSL_PARAM_BLD_free(tmpl);
    if (params != NULL)
        OSSL_PARAM_free(params);
    if (bn_mod != NULL)
        BN_free(bn_mod);
    if (bn_exp != NULL)
        BN_free(bn_exp);
#endif

    return pkey;
}

static EVP_PKEY *rsa_convert_private_key(OBJECT *key_obj)
{
    CK_ATTRIBUTE *modulus = NULL;
    CK_ATTRIBUTE *pub_exp = NULL;
    CK_ATTRIBUTE *priv_exp = NULL;
    CK_ATTRIBUTE *prime1 = NULL;
    CK_ATTRIBUTE *prime2 = NULL;
    CK_ATTRIBUTE *exp_1 = NULL;
    CK_ATTRIBUTE *exp_2 = NULL;
    CK_ATTRIBUTE *coeff = NULL;
    EVP_PKEY *pkey = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    EVP_PKEY_CTX *pctx = NULL;
    OSSL_PARAM_BLD *tmpl = NULL;
    OSSL_PARAM *params = NULL;
#else
    RSA *rsa;
#endif
    BIGNUM *bn_mod, *bn_pub_exp, *bn_priv_exp, *bn_p1, *bn_p2, *bn_e1, *bn_e2,
        *bn_cf;

    template_attribute_get_non_empty(key_obj->template, CKA_MODULUS, &modulus);
    template_attribute_get_non_empty(key_obj->template,  CKA_PUBLIC_EXPONENT,
                                     &pub_exp);
    template_attribute_find(key_obj->template, CKA_PRIVATE_EXPONENT, &priv_exp);
    template_attribute_find(key_obj->template, CKA_PRIME_1, &prime1);
    template_attribute_find(key_obj->template, CKA_PRIME_2, &prime2);
    template_attribute_find(key_obj->template, CKA_EXPONENT_1, &exp_1);
    template_attribute_find(key_obj->template, CKA_EXPONENT_2,&exp_2);
    template_attribute_find(key_obj->template, CKA_COEFFICIENT, &coeff);

    if (!prime2 && !modulus) {
        return NULL;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    // Create and init all the RSA and BIGNUM structs we need.
    rsa = RSA_new();
    if (rsa == NULL)
        return NULL;

    /*
     * Depending if an engine is loaded on OpenSSL and define its own
     * RSA_METHOD, we can end up having an infinite loop as the SOFT
     * Token doesn't implement RSA and, instead, calls OpenSSL for it.
     * So to avoid it we set RSA methods to the default rsa methods.
     */
    RSA_set_method(rsa, RSA_PKCS1_OpenSSL());
#endif

    bn_mod = BN_new();
    bn_pub_exp = BN_new();
    bn_priv_exp = BN_new();
    bn_p1 = BN_new();
    bn_p2 = BN_new();
    bn_e1 = BN_new();
    bn_e2 = BN_new();
    bn_cf = BN_new();

    if ((bn_cf == NULL) || (bn_e2 == NULL) || (bn_e1 == NULL) ||
        (bn_p2 == NULL) || (bn_p1 == NULL) || (bn_priv_exp == NULL) ||
        (bn_pub_exp == NULL) || (bn_mod == NULL))
        goto out;

    // CRT key?
    if (prime1) {
        if (!prime2 || !exp_1 || !exp_2 || !coeff)
            goto out;

        // Even though this is CRT key, OpenSSL requires the
        // modulus and exponents filled in or encrypt and decrypt will
        // not work
        BN_bin2bn((unsigned char *) modulus->pValue, modulus->ulValueLen,
                  bn_mod);
        BN_bin2bn((unsigned char *) pub_exp->pValue, pub_exp->ulValueLen,
                  bn_pub_exp);
        BN_bin2bn((unsigned char *) priv_exp->pValue, priv_exp->ulValueLen,
                  bn_priv_exp);

        BN_bin2bn((unsigned char *) prime1->pValue, prime1->ulValueLen, bn_p1);
        BN_bin2bn((unsigned char *) prime2->pValue, prime2->ulValueLen, bn_p2);

        BN_bin2bn((unsigned char *) exp_1->pValue, exp_1->ulValueLen, bn_e1);
        BN_bin2bn((unsigned char *) exp_2->pValue, exp_2->ulValueLen, bn_e2);
        BN_bin2bn((unsigned char *) coeff->pValue, coeff->ulValueLen, bn_cf);

#if !OPENSSL_VERSION_PREREQ(3, 0)
        RSA_set0_key(rsa, bn_mod, bn_pub_exp, bn_priv_exp);
        bn_mod = NULL;
        bn_pub_exp = NULL;
        bn_priv_exp = NULL;
        RSA_set0_factors(rsa, bn_p1, bn_p2);
        bn_p1 = NULL;
        bn_p2 = NULL;
        RSA_set0_crt_params(rsa, bn_e1, bn_e2, bn_cf);
        bn_e1 = NULL;
        bn_e2 = NULL;
        bn_cf = NULL;

        pkey = EVP_PKEY_new();
        if (pkey == NULL)
            goto out;

        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1)
            goto out;
#else
        tmpl = OSSL_PARAM_BLD_new();
        if (tmpl == NULL)
            goto out;

        if (!OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_N, bn_mod) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_E, bn_pub_exp) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_D, bn_priv_exp) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_FACTOR1, bn_p1) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_FACTOR2, bn_p2) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_EXPONENT1,
                                                                       bn_e1) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_EXPONENT2,
                                                                       bn_e2) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_COEFFICIENT1,
                                                                       bn_cf))
            goto out;
#endif
    } else {                    // must be a non-CRT key
        if (!priv_exp) {
            return NULL;
        }
        BN_bin2bn((unsigned char *) modulus->pValue, modulus->ulValueLen,
                  bn_mod);
        BN_bin2bn((unsigned char *) pub_exp->pValue, pub_exp->ulValueLen,
                  bn_pub_exp);
        BN_bin2bn((unsigned char *) priv_exp->pValue, priv_exp->ulValueLen,
                  bn_priv_exp);

#if !OPENSSL_VERSION_PREREQ(3, 0)
        RSA_set0_key(rsa, bn_mod, bn_pub_exp, bn_priv_exp);
        bn_mod = NULL;
        bn_pub_exp = NULL;
        bn_priv_exp = NULL;

        pkey = EVP_PKEY_new();
        if (pkey == NULL)
            goto out;

        if (EVP_PKEY_assign_RSA(pkey, rsa) != 1)
            goto out;
#else
        tmpl = OSSL_PARAM_BLD_new();
        if (tmpl == NULL)
            goto out;

        if (!OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_N, bn_mod) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_E, bn_pub_exp) ||
            !OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_RSA_D, bn_priv_exp))
            goto out;
#endif
    }

#if OPENSSL_VERSION_PREREQ(3, 0)
    params = OSSL_PARAM_BLD_to_param(tmpl);
    if (params == NULL)
        goto out;

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (pctx == NULL)
        goto out;

    if (!EVP_PKEY_fromdata_init(pctx) ||
        !EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params))
        goto out;

    EVP_PKEY_CTX_free(pctx);
    OSSL_PARAM_BLD_free(tmpl);
    OSSL_PARAM_free(params);
    BN_free(bn_mod);
    BN_free(bn_pub_exp);
    BN_free(bn_priv_exp);
    BN_free(bn_p1);
    BN_free(bn_p2);
    BN_free(bn_e1);
    BN_free(bn_e2);
    BN_free(bn_cf);
#endif

    return pkey;
out:
#if !OPENSSL_VERSION_PREREQ(3, 0)
    if (rsa)
        RSA_free(rsa);
#else
    if (pctx != NULL)
        EVP_PKEY_CTX_free(pctx);
    if (tmpl != NULL)
        OSSL_PARAM_BLD_free(tmpl);
    if (params != NULL)
        OSSL_PARAM_free(params);
#endif
    if (pkey)
        EVP_PKEY_free(pkey);
    if (bn_mod)
        BN_free(bn_mod);
    if (bn_pub_exp)
        BN_free(bn_pub_exp);
    if (bn_priv_exp)
        BN_free(bn_priv_exp);
    if (bn_p1)
        BN_free(bn_p1);
    if (bn_p2)
        BN_free(bn_p2);
    if (bn_e1)
        BN_free(bn_e1);
    if (bn_e2)
        BN_free(bn_e2);
    if (bn_cf)
        BN_free(bn_cf);

    return NULL;
}

CK_RV openssl_specific_rsa_encrypt(STDLL_TokData_t *tokdata, CK_BYTE *in_data,
                                   CK_ULONG in_data_len, CK_BYTE *out_data,
                                   OBJECT *key_obj)
{
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    CK_RV rc;
    size_t outlen = in_data_len;

    UNUSED(tokdata);

    pkey = rsa_convert_public_key(key_obj);
    if (pkey == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        return rc;
    }

    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }

    if (EVP_PKEY_encrypt_init(ctx) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_NO_PADDING) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
    if (EVP_PKEY_encrypt(ctx, out_data, &outlen,
                         in_data, in_data_len) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }

    rc = CKR_OK;
done:
    if (pkey != NULL)
        EVP_PKEY_free(pkey);
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);
    return rc;
}

CK_RV openssl_specific_rsa_decrypt(STDLL_TokData_t *tokdata, CK_BYTE *in_data,
                                   CK_ULONG in_data_len, CK_BYTE *out_data,
                                   OBJECT *key_obj)
{
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    size_t outlen = in_data_len;
    CK_RV rc;

    UNUSED(tokdata);

    pkey = rsa_convert_private_key(key_obj);
    if (pkey == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        return rc;
    }

    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }

    if (EVP_PKEY_decrypt_init(ctx) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_NO_PADDING) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }
    if (EVP_PKEY_decrypt(ctx, out_data, &outlen,
                         in_data, in_data_len) != 1) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto done;
    }

    rc = CKR_OK;
done:
    if (pkey != NULL)
        EVP_PKEY_free(pkey);
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);
    return rc;
}

CK_RV openssl_specific_rsa_pkcs_encrypt(STDLL_TokData_t *tokdata,
                                        CK_BYTE *in_data, CK_ULONG in_data_len,
                                        CK_BYTE *out_data,
                                        CK_ULONG *out_data_len,
                                        OBJECT *key_obj,
                                        t_rsa_encrypt rsa_encrypt_func)
{
    CK_RV rc;
    CK_ULONG modulus_bytes;
    CK_BYTE clear[MAX_RSA_KEYLEN], cipher[MAX_RSA_KEYLEN];
    CK_ATTRIBUTE *attr = NULL;

    /* format the data */
    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    }
    modulus_bytes = attr->ulValueLen;

    rc = rsa_format_block(tokdata, in_data, in_data_len, clear,
                          modulus_bytes, PKCS_BT_2);
    if (rc != CKR_OK) {
        TRACE_DEVEL("rsa_format_block failed\n");
        goto done;
    }
    // Do an RSA public encryption
    rc = rsa_encrypt_func(tokdata, clear, modulus_bytes, cipher, key_obj);

    if (rc == CKR_OK) {
        memcpy(out_data, cipher, modulus_bytes);
        *out_data_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed\n");
    }

done:
    OPENSSL_cleanse(clear, sizeof(clear));
    return rc;
}

CK_RV openssl_specific_rsa_pkcs_decrypt(STDLL_TokData_t *tokdata,
                                        CK_BYTE *in_data, CK_ULONG in_data_len,
                                        CK_BYTE *out_data,
                                        CK_ULONG *out_data_len, OBJECT *key_obj,
                                        t_rsa_decrypt rsa_decrypt_func)
{
    CK_RV rc;
    CK_BYTE out[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;

    modulus_bytes = in_data_len;

    rc = rsa_decrypt_func(tokdata, in_data, modulus_bytes, out, key_obj);
    if (rc != CKR_OK) {
        TRACE_DEVEL("openssl_specific_rsa_decrypt failed\n");
        goto done;
    }

    rc = rsa_parse_block(out, modulus_bytes, out_data, out_data_len, PKCS_BT_2);
    if (rc != CKR_OK) {
        TRACE_DEVEL("rsa_parse_block failed\n");
        goto done;
    }

    /*
     * For PKCS #1 v1.5 padding, out_data_len must be less than
     * modulus_bytes - 11.
     */
    if (*out_data_len > (modulus_bytes - 11)) {
        TRACE_ERROR("%s\n", ock_err(ERR_ENCRYPTED_DATA_LEN_RANGE));
        rc = CKR_ENCRYPTED_DATA_LEN_RANGE;
    }

done:
    OPENSSL_cleanse(out, sizeof(out));
    return rc;
}


CK_RV openssl_specific_rsa_pkcs_sign(STDLL_TokData_t *tokdata, SESSION *sess,
                                     CK_BYTE *in_data, CK_ULONG in_data_len,
                                     CK_BYTE *signature, CK_ULONG *sig_len,
                                     OBJECT *key_obj,
                                     t_rsa_decrypt rsa_decrypt_func)
{
    CK_BYTE data[MAX_RSA_KEYLEN], sig[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;
    CK_ATTRIBUTE *attr = NULL;

    UNUSED(sess);

    /* format the data */
    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    }
    modulus_bytes = attr->ulValueLen;
    rc = rsa_format_block(tokdata, in_data, in_data_len, data,
                          modulus_bytes, PKCS_BT_1);
    if (rc != CKR_OK) {
        TRACE_DEVEL("rsa_format_block failed\n");
        return rc;
    }

    /* signing is a private key operation --> decrypt */
    rc = rsa_decrypt_func(tokdata, data, modulus_bytes, sig, key_obj);
    if (rc == CKR_OK) {
        memcpy(signature, sig, modulus_bytes);
        *sig_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_decrypt failed\n");
    }

    return rc;
}

CK_RV openssl_specific_rsa_pkcs_verify(STDLL_TokData_t *tokdata, SESSION *sess,
                                       CK_BYTE *in_data, CK_ULONG in_data_len,
                                       CK_BYTE *signature, CK_ULONG sig_len,
                                       OBJECT *key_obj,
                                       t_rsa_encrypt rsa_encrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE out[MAX_RSA_KEYLEN], out_data[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes, out_data_len;
    CK_RV rc;

    UNUSED(sess);
    UNUSED(sig_len);

    out_data_len = MAX_RSA_KEYLEN;
    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    } else {
        modulus_bytes = attr->ulValueLen;
    }

    // verifying is a public key operation --> encrypt
    //
    rc = rsa_encrypt_func(tokdata, signature, modulus_bytes, out, key_obj);
    if (rc != CKR_OK) {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed: %lx\n", rc);
        /*
         * Return CKR_SIGNATURE_INVALID in case of CKR_ARGUMENTS_BAD or
         * CKR_FUNCTION_FAILED because we dont know why the RSA op failed and
         * it may have failed due to a tampered signature being greater or equal
         * to the modulus.
         */
        if (rc == CKR_ARGUMENTS_BAD || rc == CKR_FUNCTION_FAILED) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            rc = CKR_SIGNATURE_INVALID;
        }
        return rc;
    }

    rc = rsa_parse_block(out, modulus_bytes, out_data, &out_data_len,
                         PKCS_BT_1);
    if (rc == CKR_ENCRYPTED_DATA_INVALID) {
        TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
        return CKR_SIGNATURE_INVALID;
    } else if (rc != CKR_OK) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }

    if (in_data_len != out_data_len) {
        TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
        return CKR_SIGNATURE_INVALID;
    }

    if (CRYPTO_memcmp(in_data, out_data, out_data_len) != 0) {
        TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
        return CKR_SIGNATURE_INVALID;
    }

    return rc;
}

CK_RV openssl_specific_rsa_pkcs_verify_recover(STDLL_TokData_t *tokdata,
                                               CK_BYTE *signature,
                                               CK_ULONG sig_len,
                                               CK_BYTE *out_data,
                                               CK_ULONG *out_data_len,
                                               OBJECT *key_obj,
                                               t_rsa_encrypt rsa_encrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE out[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;

    UNUSED(sig_len);

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    } else {
        modulus_bytes = attr->ulValueLen;
    }

    // verifying is a public key operation --> encrypt
    //
    rc = rsa_encrypt_func(tokdata, signature, modulus_bytes, out, key_obj);
    if (rc != CKR_OK) {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed: %lx\n", rc);
        /*
         * Return CKR_SIGNATURE_INVALID in case of CKR_ARGUMENTS_BAD or
         * CKR_FUNCTION_FAILED because we dont know why the RSA op failed and
         * it may have failed due to a tampered signature being greater or equal
         * to the modulus.
         */
        if (rc == CKR_ARGUMENTS_BAD || rc == CKR_FUNCTION_FAILED) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            rc = CKR_SIGNATURE_INVALID;
        }
        return rc;
    }

    rc = rsa_parse_block(out, modulus_bytes, out_data, out_data_len, PKCS_BT_1);
    if (rc == CKR_ENCRYPTED_DATA_INVALID) {
        TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
        return CKR_SIGNATURE_INVALID;
    } else if (rc != CKR_OK) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
    }

    return rc;
}

CK_RV openssl_specific_rsa_pss_sign(STDLL_TokData_t *tokdata, SESSION *sess,
                                    SIGN_VERIFY_CONTEXT *ctx,
                                    CK_BYTE *in_data, CK_ULONG in_data_len,
                                    CK_BYTE *sig, CK_ULONG *sig_len,
                                    t_rsa_decrypt rsa_decrypt_func)
{
    CK_RV rc;
    CK_ULONG modbytes;
    CK_ATTRIBUTE *attr = NULL;
    OBJECT *key_obj = NULL;
    CK_BYTE *emdata = NULL;
    CK_RSA_PKCS_PSS_PARAMS *pssParms = NULL;

    UNUSED(sess);

    /* check the arguments */
    if (!in_data || !sig) {
        TRACE_ERROR("%s\n", ock_err(ERR_ARGUMENTS_BAD));
        return CKR_ARGUMENTS_BAD;
    }

    if (!ctx) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }

    pssParms = (CK_RSA_PKCS_PSS_PARAMS *) ctx->mech.pParameter;

    /* get the key */
    rc = object_mgr_find_in_map1(tokdata, ctx->key, &key_obj, READ_LOCK);
    if (rc != CKR_OK) {
        TRACE_DEVEL("object_mgr_find_in_map1 failed\n");
        return rc;
    }

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
   if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        goto done;
    } else {
        modbytes = attr->ulValueLen;
    }

    emdata = (CK_BYTE *) malloc(modbytes);
    if (emdata == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }

    rc = emsa_pss_encode(tokdata, pssParms, in_data, in_data_len, emdata,
                         &modbytes);
    if (rc != CKR_OK)
        goto done;

    /* signing is a private key operation --> decrypt  */
    rc = rsa_decrypt_func(tokdata, emdata, modbytes, sig, key_obj);
    if (rc == CKR_OK)
        *sig_len = modbytes;
    else
        TRACE_DEVEL("openssl_specific_rsa_decrypt failed\n");

done:
    if (emdata)
        free(emdata);

    object_put(tokdata, key_obj, TRUE);
    key_obj = NULL;

    return rc;
}


CK_RV openssl_specific_rsa_pss_verify(STDLL_TokData_t *tokdata, SESSION *sess,
                                      SIGN_VERIFY_CONTEXT *ctx,
                                      CK_BYTE *in_data, CK_ULONG in_data_len,
                                      CK_BYTE *signature, CK_ULONG sig_len,
                                      t_rsa_encrypt rsa_encrypt_func)
{
    CK_RV rc;
    CK_ULONG modbytes;
    OBJECT *key_obj = NULL;
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE out[MAX_RSA_KEYLEN];
    CK_RSA_PKCS_PSS_PARAMS *pssParms = NULL;

    UNUSED(sess);

    /* check the arguments */
    if (!in_data || !signature) {
        TRACE_ERROR("%s\n", ock_err(ERR_ARGUMENTS_BAD));
        return CKR_ARGUMENTS_BAD;
    }

    if (!ctx) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }

    pssParms = (CK_RSA_PKCS_PSS_PARAMS *) ctx->mech.pParameter;

    /* get the key */
    rc = object_mgr_find_in_map1(tokdata, ctx->key, &key_obj, READ_LOCK);
    if (rc != CKR_OK) {
        TRACE_DEVEL("object_mgr_find_in_map1 failed\n");
        return rc;
    }

    /* verify is a public key operation ... encrypt */
    rc = rsa_encrypt_func(tokdata, signature, sig_len, out, key_obj);
    if (rc != CKR_OK) {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed: %lx\n", rc);
        /*
         * Return CKR_SIGNATURE_INVALID in case of CKR_ARGUMENTS_BAD or
         * CKR_FUNCTION_FAILED because we dont know why the RSA op failed and
         * it may have failed due to a tampered signature being greater or equal
         * to the modulus.
         */
        if (rc == CKR_ARGUMENTS_BAD || rc == CKR_FUNCTION_FAILED) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            rc = CKR_SIGNATURE_INVALID;
        }
        goto done;
    }

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        goto done;
    } else {
        modbytes = attr->ulValueLen;
    }

    /* call the pss verify scheme */
    rc = emsa_pss_verify(tokdata, pssParms, in_data, in_data_len, out,
                         modbytes);

done:
    object_put(tokdata, key_obj, TRUE);
    key_obj = NULL;

    return rc;
}


CK_RV openssl_specific_rsa_x509_encrypt(STDLL_TokData_t *tokdata,
                                        CK_BYTE *in_data, CK_ULONG in_data_len,
                                        CK_BYTE *out_data,
                                        CK_ULONG *out_data_len, OBJECT *key_obj,
                                        t_rsa_encrypt rsa_encrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE clear[MAX_RSA_KEYLEN], cipher[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        goto done;
    }

    modulus_bytes = attr->ulValueLen;

    // prepad with zeros
    //
    memset(clear, 0, modulus_bytes - in_data_len);
    memcpy(&clear[modulus_bytes - in_data_len], in_data, in_data_len);

    rc = rsa_encrypt_func(tokdata, clear, modulus_bytes, cipher, key_obj);
    if (rc == CKR_OK) {
        memcpy(out_data, cipher, modulus_bytes);
        *out_data_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed\n");
    }

done:
    OPENSSL_cleanse(clear, sizeof(clear));
    return rc;
}

CK_RV openssl_specific_rsa_x509_decrypt(STDLL_TokData_t *tokdata,
                                        CK_BYTE *in_data, CK_ULONG in_data_len,
                                        CK_BYTE *out_data,
                                        CK_ULONG *out_data_len, OBJECT *key_obj,
                                        t_rsa_decrypt rsa_decrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE out[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;

    UNUSED(in_data_len);

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        goto done;
    }

    modulus_bytes = attr->ulValueLen;

    rc = rsa_decrypt_func(tokdata, in_data, modulus_bytes, out, key_obj);
    if (rc == CKR_OK) {
        memcpy(out_data, out, modulus_bytes);
        *out_data_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_decrypt failed\n");
    }

done:
    OPENSSL_cleanse(out, sizeof(out));
    return rc;
}


CK_RV openssl_specific_rsa_x509_sign(STDLL_TokData_t *tokdata, CK_BYTE *in_data,
                                     CK_ULONG in_data_len, CK_BYTE *signature,
                                     CK_ULONG *sig_len, OBJECT *key_obj,
                                     t_rsa_decrypt rsa_decrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE data[MAX_RSA_KEYLEN], sig[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    } else {
        modulus_bytes = attr->ulValueLen;
    }

    // prepad with zeros
    //

    memset(data, 0x0, modulus_bytes - in_data_len);
    memcpy(&data[modulus_bytes - in_data_len], in_data, in_data_len);

    rc = rsa_decrypt_func(tokdata, data, modulus_bytes, sig, key_obj);
    if (rc == CKR_OK) {
        memcpy(signature, sig, modulus_bytes);
        *sig_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed: %lx\n", rc);
        /*
         * Return CKR_SIGNATURE_INVALID in case of CKR_ARGUMENTS_BAD or
         * CKR_FUNCTION_FAILED because we dont know why the RSA op failed and
         * it may have failed due to a tampered signature being greater or equal
         * to the modulus.
         */
        if (rc == CKR_ARGUMENTS_BAD || rc == CKR_FUNCTION_FAILED) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            rc = CKR_SIGNATURE_INVALID;
        }
    }

    return rc;
}

CK_RV openssl_specific_rsa_x509_verify(STDLL_TokData_t *tokdata,
                                       CK_BYTE *in_data, CK_ULONG in_data_len,
                                       CK_BYTE *signature, CK_ULONG sig_len,
                                       OBJECT *key_obj,
                                       t_rsa_encrypt rsa_encrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE out[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;

    UNUSED(sig_len);

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    } else {
        modulus_bytes = attr->ulValueLen;
    }

    rc = rsa_encrypt_func(tokdata, signature, modulus_bytes, out, key_obj);
    if (rc == CKR_OK) {
        CK_ULONG pos1, pos2, len;
        // it should be noted that in_data_len is not necessarily
        // the same as the modulus length
        //
        for (pos1 = 0; pos1 < in_data_len; pos1++)
            if (in_data[pos1] != 0)
                break;

        for (pos2 = 0; pos2 < modulus_bytes; pos2++)
            if (out[pos2] != 0)
                break;

        // at this point, pos1 and pos2 point to the first non-zero
        // bytes in the input data and the decrypted signature
        // (the recovered data), respectively.
        if ((in_data_len - pos1) != (modulus_bytes - pos2)) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            return CKR_SIGNATURE_INVALID;
        }
        len = in_data_len - pos1;

        if (CRYPTO_memcmp(&in_data[pos1], &out[pos2], len) != 0) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            return CKR_SIGNATURE_INVALID;
        }
        return CKR_OK;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed: %lx\n", rc);
        /*
         * Return CKR_SIGNATURE_INVALID in case of CKR_ARGUMENTS_BAD or
         * CKR_FUNCTION_FAILED because we dont know why the RSA op failed and
         * it may have failed due to a tampered signature being greater or equal
         * to the modulus.
         */
        if (rc == CKR_ARGUMENTS_BAD || rc == CKR_FUNCTION_FAILED) {
            TRACE_ERROR("%s\n", ock_err(ERR_SIGNATURE_INVALID));
            rc = CKR_SIGNATURE_INVALID;
        }
    }

    return rc;
}

CK_RV openssl_specific_rsa_x509_verify_recover(STDLL_TokData_t *tokdata,
                                               CK_BYTE *signature,
                                               CK_ULONG sig_len,
                                               CK_BYTE *out_data,
                                               CK_ULONG *out_data_len,
                                               OBJECT *key_obj,
                                               t_rsa_encrypt rsa_encrypt_func)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE out[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_RV rc;

    UNUSED(sig_len);

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        return rc;
    } else {
        modulus_bytes = attr->ulValueLen;
    }

    rc = rsa_encrypt_func(tokdata, signature, modulus_bytes, out, key_obj);
    if (rc == CKR_OK) {
        memcpy(out_data, out, modulus_bytes);
        *out_data_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed\n");
    }

    return rc;
}

CK_RV openssl_specific_rsa_oaep_encrypt(STDLL_TokData_t *tokdata,
                                        ENCR_DECR_CONTEXT *ctx,
                                        CK_BYTE *in_data, CK_ULONG in_data_len,
                                        CK_BYTE *out_data,
                                        CK_ULONG *out_data_len, CK_BYTE *hash,
                                        CK_ULONG hlen,
                                        t_rsa_encrypt rsa_encrypt_func)
{
    CK_RV rc;
    CK_BYTE cipher[MAX_RSA_KEYLEN];
    CK_ULONG modulus_bytes;
    CK_ATTRIBUTE *attr = NULL;
    CK_BYTE *em_data = NULL;
    OBJECT *key_obj = NULL;
    CK_RSA_PKCS_OAEP_PARAMS_PTR oaepParms = NULL;

    if (!in_data || !out_data || !hash) {
        TRACE_ERROR("%s\n", ock_err(ERR_ARGUMENTS_BAD));
        return CKR_ARGUMENTS_BAD;
    }

    oaepParms = (CK_RSA_PKCS_OAEP_PARAMS_PTR) ctx->mech.pParameter;

    rc = object_mgr_find_in_map1(tokdata, ctx->key, &key_obj, READ_LOCK);
    if (rc != CKR_OK) {
        TRACE_DEVEL("object_mgr_find_in_map1 failed\n");
        return rc;
    }

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        goto done;
    }

    modulus_bytes = attr->ulValueLen;

    /* pkcs1v2.2, section 7.1.1 Step 2:
     * EME-OAEP encoding.
     */
    em_data = (CK_BYTE *) malloc(modulus_bytes * sizeof(CK_BYTE));
    if (em_data == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto done;
    }

    rc = encode_eme_oaep(tokdata, in_data, in_data_len, em_data,
                         modulus_bytes, oaepParms->mgf, hash, hlen);
    if (rc != CKR_OK)
        goto done;

    rc = rsa_encrypt_func(tokdata, em_data, modulus_bytes, cipher, key_obj);
    if (rc == CKR_OK) {
        memcpy(out_data, cipher, modulus_bytes);
        *out_data_len = modulus_bytes;
    } else {
        TRACE_DEVEL("openssl_specific_rsa_encrypt failed\n");
    }

done:
    if (em_data) {
        OPENSSL_cleanse(em_data, modulus_bytes * sizeof(CK_BYTE));
        free(em_data);
    }

    object_put(tokdata, key_obj, TRUE);
    key_obj = NULL;

    return rc;
}

CK_RV openssl_specific_rsa_oaep_decrypt(STDLL_TokData_t *tokdata,
                                        ENCR_DECR_CONTEXT *ctx,
                                        CK_BYTE *in_data, CK_ULONG in_data_len,
                                        CK_BYTE *out_data,
                                        CK_ULONG *out_data_len, CK_BYTE *hash,
                                        CK_ULONG hlen,
                                        t_rsa_encrypt rsa_decrypt_func)
{
    CK_RV rc;
    CK_BYTE *decr_data = NULL;
    OBJECT *key_obj = NULL;
    CK_ATTRIBUTE *attr = NULL;
    CK_RSA_PKCS_OAEP_PARAMS_PTR oaepParms = NULL;

    if (!in_data || !out_data || !hash) {
        TRACE_ERROR("%s\n", ock_err(ERR_ARGUMENTS_BAD));
        return CKR_ARGUMENTS_BAD;
    }

    oaepParms = (CK_RSA_PKCS_OAEP_PARAMS_PTR) ctx->mech.pParameter;

    rc = object_mgr_find_in_map1(tokdata, ctx->key, &key_obj, READ_LOCK);
    if (rc != CKR_OK) {
        TRACE_DEVEL("object_mgr_find_in_map1 failed\n");
        return rc;
    }

    rc = template_attribute_get_non_empty(key_obj->template, CKA_MODULUS,
                                          &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_MODULUS for the key.\n");
        goto error;
    }

    *out_data_len = attr->ulValueLen;

    decr_data = (CK_BYTE *) malloc(in_data_len);
    if (decr_data == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        rc = CKR_HOST_MEMORY;
        goto error;
    }

    rc = rsa_decrypt_func(tokdata, in_data, in_data_len, decr_data, key_obj);
    if (rc != CKR_OK)
        goto error;

    /* pkcs1v2.2, section 7.1.2 Step 2:
     * EME-OAEP decoding.
     */
    rc = decode_eme_oaep(tokdata, decr_data, in_data_len, out_data,
                         out_data_len, oaepParms->mgf, hash, hlen);

error:
    if (decr_data) {
        OPENSSL_cleanse(decr_data, in_data_len);
        free(decr_data);
    }

    object_put(tokdata, key_obj, TRUE);
    key_obj = NULL;

    return rc;
}


#ifndef NO_EC

static int curve_nid_from_params(const CK_BYTE *params, CK_ULONG params_len)
{
    const unsigned char *oid;
    ASN1_OBJECT *obj = NULL;
    int nid;

    oid = params;
    obj = d2i_ASN1_OBJECT(NULL, &oid, params_len);
    if (obj == NULL || oid != params + params_len) {
        TRACE_ERROR("curve not supported by OpenSSL.\n");
        return NID_undef;
    }

    nid = OBJ_obj2nid(obj);
    ASN1_OBJECT_free(obj);

    return nid;
}

static int ec_prime_len_from_nid(int nid)
{
    EC_GROUP *group;
    int primelen;

    group = EC_GROUP_new_by_curve_name(nid);
    if (group == NULL)
        return -1;

    primelen = EC_GROUP_order_bits(group);

    EC_GROUP_free(group);

    return (primelen + 7) / 8;
}

static int ec_prime_len_from_pkey(EVP_PKEY *pkey)
{
#if !OPENSSL_VERSION_PREREQ(3, 0)
    return (EC_GROUP_order_bits(EC_KEY_get0_group(
                             EVP_PKEY_get0_EC_KEY(pkey))) + 7) / 8;
#else
    size_t curve_len;
    char curve[80];

    if (!EVP_PKEY_get_utf8_string_param(pkey, OSSL_PKEY_PARAM_GROUP_NAME,
                                        curve, sizeof(curve), &curve_len))
        return -1;

    return ec_prime_len_from_nid(OBJ_sn2nid(curve));
#endif
}


#if !OPENSSL_VERSION_PREREQ(3, 0)
static CK_RV make_ec_key_from_params(const CK_BYTE *params, CK_ULONG params_len,
                                     EC_KEY **key)
{
    EC_KEY *ec_key = NULL;
    int nid;
    CK_RV rc = CKR_OK;

    nid = curve_nid_from_params(params, params_len);
    if (nid == NID_undef) {
        TRACE_ERROR("curve not supported by OpenSSL.\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

    ec_key = EC_KEY_new_by_curve_name(nid);
    if (ec_key == NULL) {
       TRACE_ERROR("curve not supported by OpenSSL.\n");
       rc = CKR_CURVE_NOT_SUPPORTED;
       goto out;
    }

out:
    if (rc != CKR_OK) {
        if (ec_key != NULL)
            EC_KEY_free(ec_key);

        return rc;
    }

    *key = ec_key;

    return CKR_OK;
}
#endif

#if OPENSSL_VERSION_PREREQ(3, 0)
static CK_RV build_pkey_from_params(OSSL_PARAM_BLD *tmpl, int selection,
                                    EVP_PKEY **pkey)
{

    OSSL_PARAM *params = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    CK_RV rc = CKR_OK;

    params = OSSL_PARAM_BLD_to_param(tmpl);
    if (params == NULL) {
        TRACE_ERROR("OSSL_PARAM_BLD_to_param failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (pctx == NULL) {
        TRACE_ERROR("EVP_PKEY_CTX_new_id failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!EVP_PKEY_fromdata_init(pctx) ||
        !EVP_PKEY_fromdata(pctx, pkey, selection, params)) {
        TRACE_ERROR("EVP_PKEY_fromdata failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    EVP_PKEY_CTX_free(pctx);
    pctx = EVP_PKEY_CTX_new(*pkey, NULL);
    if (pctx == NULL) {
        TRACE_ERROR("EVP_PKEY_CTX_new failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (selection & OSSL_KEYMGMT_SELECT_PRIVATE_KEY) {
        if (EVP_PKEY_check(pctx) != 1) {
            TRACE_ERROR("EVP_PKEY_check failed\n");
            rc = CKR_FUNCTION_FAILED;
            goto out;
        }
    } else {
        if (EVP_PKEY_public_check(pctx) != 1) {
            TRACE_ERROR("EVP_PKEY_public_check failed\n");
            rc = CKR_FUNCTION_FAILED;
            goto out;
        }
    }

out:
    if (pctx != NULL)
        EVP_PKEY_CTX_free(pctx);
    if (params != NULL)
        OSSL_PARAM_free(params);

    if (rc != 0 && *pkey != NULL) {
        EVP_PKEY_free(*pkey);
        *pkey = NULL;
    }

    return rc;
}
#endif

#if !OPENSSL_VERSION_PREREQ(3, 0)
static CK_RV fill_ec_key_from_pubkey(EC_KEY *ec_key, const CK_BYTE *data,
                                     CK_ULONG data_len, CK_BBOOL allow_raw,
                                     int nid, EVP_PKEY **ec_pkey)
#else
static CK_RV fill_ec_key_from_pubkey(OSSL_PARAM_BLD *tmpl, const CK_BYTE *data,
                                     CK_ULONG data_len, CK_BBOOL allow_raw,
                                     int nid, EVP_PKEY **ec_pkey)
#endif
{
    CK_BYTE *ecpoint = NULL;
    CK_ULONG ecpoint_len, privlen;
    CK_BBOOL allocated = FALSE;

    CK_RV rc;

    privlen = ec_prime_len_from_nid(nid);
    if (privlen <= 0) {
        TRACE_ERROR("ec_prime_len_from_nid failed\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

    rc = ec_point_from_public_data(data, data_len, privlen, allow_raw,
                                   &allocated, &ecpoint, &ecpoint_len);
    if (rc != CKR_OK) {
        TRACE_DEVEL("ec_point_from_public_data failed\n");
        goto out;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    if (!EC_KEY_oct2key(ec_key, ecpoint, ecpoint_len, NULL)) {
        TRACE_ERROR("EC_KEY_oct2key failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!EC_KEY_check_key(ec_key)) {
        TRACE_ERROR("EC_KEY_check_key failed\n");
        rc = CKR_PUBLIC_KEY_INVALID;
        goto out;
    }

    *ec_pkey = EVP_PKEY_new();
    if (*ec_pkey == NULL) {
       TRACE_ERROR("EVP_PKEY_CTX_new failed.\n");
       rc = CKR_HOST_MEMORY;
       goto out;
    }

    if (!EVP_PKEY_assign_EC_KEY(*ec_pkey, ec_key)) {
        TRACE_ERROR("EVP_PKEY_assign_EC_KEY failed.\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#else
    if (!OSSL_PARAM_BLD_push_octet_string(tmpl,
                                          OSSL_PKEY_PARAM_PUB_KEY,
                                          ecpoint, ecpoint_len)) {
        TRACE_ERROR("OSSL_PARAM_BLD_push_octet_string failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    rc = build_pkey_from_params(tmpl, EVP_PKEY_PUBLIC_KEY, ec_pkey);
    if (rc != CKR_OK) {
        TRACE_ERROR("build_pkey_from_params failed\n");
        goto out;
    }
 #endif

out:
    if (allocated && ecpoint != NULL)
        free(ecpoint);

    return rc;
}

#if !OPENSSL_VERSION_PREREQ(3, 0)
static CK_RV fill_ec_key_from_privkey(EC_KEY *ec_key, const CK_BYTE *data,
                                      CK_ULONG data_len, EVP_PKEY **ec_pkey)
#else
static CK_RV fill_ec_key_from_privkey(OSSL_PARAM_BLD *tmpl, const CK_BYTE *data,
                                      CK_ULONG data_len, int nid,
                                      EVP_PKEY **ec_pkey)
#endif
{
    EC_POINT *point = NULL;
#if OPENSSL_VERSION_PREREQ(3, 0)
    EC_GROUP *group = NULL;
    BIGNUM *bn_priv = NULL;
    unsigned char *pub_key = NULL;
    unsigned int pub_key_len;
    point_conversion_form_t form;
#endif
    CK_RV rc = CKR_OK;

#if !OPENSSL_VERSION_PREREQ(3, 0)
    if (!EC_KEY_oct2priv(ec_key, data, data_len)) {
        TRACE_ERROR("EC_KEY_oct2priv failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    point = EC_POINT_new(EC_KEY_get0_group(ec_key));
    if (point == NULL) {
        TRACE_ERROR("EC_POINT_new failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!EC_POINT_mul(EC_KEY_get0_group(ec_key), point,
                      EC_KEY_get0_private_key(ec_key), NULL, NULL, NULL)) {
        TRACE_ERROR("EC_POINT_mul failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!EC_KEY_set_public_key(ec_key, point)) {
        TRACE_ERROR("EC_KEY_set_public_key failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!EC_KEY_check_key(ec_key)) {
        TRACE_ERROR("EC_KEY_check_key failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    *ec_pkey = EVP_PKEY_new();
    if (*ec_pkey == NULL) {
       TRACE_ERROR("EVP_PKEY_CTX_new failed.\n");
       rc = CKR_HOST_MEMORY;
       goto out;
    }

    if (!EVP_PKEY_assign_EC_KEY(*ec_pkey, ec_key)) {
        TRACE_ERROR("EVP_PKEY_assign_EC_KEY failed.\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#else
    group = EC_GROUP_new_by_curve_name(nid);
    if (group == NULL) {
        TRACE_ERROR("EC_GROUP_new_by_curve_name failed\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

    point = EC_POINT_new(group);
    if (point == NULL) {
        TRACE_ERROR("EC_POINT_new failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    bn_priv = BN_bin2bn(data, data_len, NULL);
    if (bn_priv == NULL) {
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!EC_POINT_mul(group, point, bn_priv, NULL, NULL, NULL)) {
        TRACE_ERROR("EC_POINT_mul failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    form = EC_GROUP_get_point_conversion_form(group);
    pub_key_len = EC_POINT_point2buf(group, point, form, &pub_key,
                                     NULL);
    if (pub_key_len == 0) {
        TRACE_ERROR("EC_POINT_point2buf failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!OSSL_PARAM_BLD_push_octet_string(tmpl, OSSL_PKEY_PARAM_PUB_KEY,
                                          pub_key, pub_key_len)) {
        TRACE_ERROR("OSSL_PARAM_BLD_push_octet_string failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!OSSL_PARAM_BLD_push_BN(tmpl, OSSL_PKEY_PARAM_PRIV_KEY, bn_priv)) {
        TRACE_ERROR("OSSL_PARAM_BLD_push_BN failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    rc = build_pkey_from_params(tmpl, EVP_PKEY_KEYPAIR, ec_pkey);
    if (rc != CKR_OK) {
        TRACE_ERROR("build_pkey_from_params failed\n");
        goto out;
    }
#endif

out:
    if (point != NULL)
        EC_POINT_free(point);
#if OPENSSL_VERSION_PREREQ(3, 0)
    if (group != NULL)
        EC_GROUP_free(group);
    if (bn_priv != NULL)
        BN_free(bn_priv);
    if (pub_key != NULL)
        OPENSSL_free(pub_key);
#endif

    return rc;
}

CK_RV openssl_make_ec_key_from_template(TEMPLATE *template, EVP_PKEY **pkey)
{
    CK_ATTRIBUTE *attr = NULL;
    CK_OBJECT_CLASS keyclass;
    EVP_PKEY *ec_pkey = NULL;
    int nid;
#if !OPENSSL_VERSION_PREREQ(3, 0)
    EC_KEY *ec_key = NULL;
#else
    OSSL_PARAM_BLD *tmpl = NULL;
#endif
    CK_RV rc;

    rc = template_attribute_get_ulong(template, CKA_CLASS, &keyclass);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_CLASS in the template\n");
        goto out;
    }

    rc = template_attribute_get_non_empty(template, CKA_ECDSA_PARAMS, &attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("Could not find CKA_ECDSA_PARAMS in the template\n");
        goto out;
    }

    nid = curve_nid_from_params(attr->pValue, attr->ulValueLen);
    if (nid == NID_undef) {
        TRACE_ERROR("curve not supported by OpenSSL.\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    rc = make_ec_key_from_params(attr->pValue, attr->ulValueLen, &ec_key);
    if (rc != CKR_OK)
        goto out;
#else
    tmpl = OSSL_PARAM_BLD_new();
    if (tmpl == NULL) {
        TRACE_ERROR("OSSL_PARAM_BLD_new failed\n");
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    if (!OSSL_PARAM_BLD_push_utf8_string(tmpl, OSSL_PKEY_PARAM_GROUP_NAME,
                                         OBJ_nid2sn(nid), 0)) {
        TRACE_ERROR("OSSL_PARAM_BLD_push_utf8_string failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#endif

    switch (keyclass) {
    case CKO_PUBLIC_KEY:
        rc = template_attribute_get_non_empty(template, CKA_EC_POINT, &attr);
        if (rc != CKR_OK) {
            TRACE_ERROR("Could not find CKA_EC_POINT in the template\n");
            goto out;
        }

#if !OPENSSL_VERSION_PREREQ(3, 0)
        rc = fill_ec_key_from_pubkey(ec_key, attr->pValue, attr->ulValueLen,
                                     FALSE, nid, &ec_pkey);
#else
        rc = fill_ec_key_from_pubkey(tmpl, attr->pValue, attr->ulValueLen,
                                     FALSE, nid, &ec_pkey);
#endif
        if (rc != CKR_OK) {
            TRACE_DEVEL("fill_ec_key_from_pubkey failed\n");
            goto out;
        }
        break;

    case CKO_PRIVATE_KEY:
        rc = template_attribute_get_non_empty(template, CKA_VALUE, &attr);
        if (rc != CKR_OK) {
            TRACE_ERROR("Could not find CKA_VALUE in the template\n");
            goto out;
        }

#if !OPENSSL_VERSION_PREREQ(3, 0)
        rc = fill_ec_key_from_privkey(ec_key, attr->pValue, attr->ulValueLen,
                                      &ec_pkey);
#else
        rc = fill_ec_key_from_privkey(tmpl, attr->pValue, attr->ulValueLen,
                                      nid, &ec_pkey);

#endif
        if (rc != CKR_OK) {
            TRACE_DEVEL("fill_ec_key_from_privkey failed\n");
            goto out;
        }
        break;

    default:
        rc = CKR_KEY_FUNCTION_NOT_PERMITTED;
        goto out;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    ec_key = NULL;
#endif

    rc = CKR_OK;

out:
#if OPENSSL_VERSION_PREREQ(3, 0)
    if (tmpl != NULL)
        OSSL_PARAM_BLD_free(tmpl);
#endif

    if (rc != CKR_OK) {
        if (ec_pkey != NULL)
            EVP_PKEY_free(ec_pkey);
#if !OPENSSL_VERSION_PREREQ(3, 0)
        if (ec_key != NULL)
            EC_KEY_free(ec_key);
#endif

        return rc;
    }

    *pkey = ec_pkey;

    return CKR_OK;
}

CK_RV openssl_specific_ec_generate_keypair(STDLL_TokData_t *tokdata,
                                           TEMPLATE *publ_tmpl,
                                           TEMPLATE *priv_tmpl)
{

    CK_ATTRIBUTE *attr = NULL, *ec_point_attr, *value_attr, *parms_attr;
#if !OPENSSL_VERSION_PREREQ(3, 0)
    const EC_KEY *ec_key = NULL;
    BN_CTX *bnctx = NULL;
#else
    BIGNUM *bn_d = NULL;
#endif
    CK_BYTE *ecpoint = NULL, *enc_ecpoint = NULL, *d = NULL;
    CK_ULONG ecpoint_len, enc_ecpoint_len, d_len;
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *ec_pkey = NULL;
    int nid;
    CK_RV rc;

    UNUSED(tokdata);

    rc = template_attribute_get_non_empty(publ_tmpl, CKA_ECDSA_PARAMS, &attr);
    if (rc != CKR_OK)
        goto out;

    nid = curve_nid_from_params(attr->pValue, attr->ulValueLen);
    if (nid == NID_undef) {
        TRACE_ERROR("curve not supported by OpenSSL.\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (ctx == NULL) {
        TRACE_ERROR("EVP_PKEY_CTX_new failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        TRACE_ERROR("EVP_PKEY_keygen_init failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, nid) <= 0) {
        TRACE_ERROR("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

    if (EVP_PKEY_keygen(ctx, &ec_pkey) <= 0) {
        TRACE_ERROR("EVP_PKEY_keygen failed\n");
        if (ERR_GET_REASON(ERR_peek_last_error()) == EC_R_INVALID_CURVE)
            rc = CKR_CURVE_NOT_SUPPORTED;
        else
            rc = CKR_FUNCTION_FAILED;
        goto out;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    ec_key = EVP_PKEY_get0_EC_KEY(ec_pkey);
    if (ec_key == NULL) {
       TRACE_ERROR("EVP_PKEY_get0_EC_KEY failed\n");
       rc = CKR_FUNCTION_FAILED;
       goto out;
   }

    bnctx = BN_CTX_new();
    if (bnctx == NULL) {
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    ecpoint_len = EC_KEY_key2buf(ec_key, POINT_CONVERSION_UNCOMPRESSED,
                                 &ecpoint, bnctx);
    if (ecpoint_len == 0) {
        TRACE_ERROR("Failed to get the EC Point compressed.\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#else
    if (!EVP_PKEY_get_octet_string_param(ec_pkey,
                                         OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY,
                                         NULL, 0, &ecpoint_len)) {
        TRACE_ERROR("EVP_PKEY_get_octet_string_param failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    ecpoint = OPENSSL_zalloc(ecpoint_len);
    if (ecpoint == NULL) {
        TRACE_ERROR("OPENSSL_zalloc failed\n");
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    if (!EVP_PKEY_get_octet_string_param(ec_pkey,
                                         OSSL_PKEY_PARAM_ENCODED_PUBLIC_KEY,
                                         ecpoint, ecpoint_len, &ecpoint_len)) {
        TRACE_ERROR("EVP_PKEY_get_octet_string_param failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#endif

    rc = ber_encode_OCTET_STRING(FALSE, &enc_ecpoint, &enc_ecpoint_len,
                                 ecpoint, ecpoint_len);
    if (rc != CKR_OK) {
        TRACE_DEVEL("ber_encode_OCTET_STRING failed\n");
        goto out;
    }

    rc = build_attribute(CKA_EC_POINT, enc_ecpoint, enc_ecpoint_len,
                         &ec_point_attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("build_attribute for CKA_EC_POINT failed rc=0x%lx\n", rc);
        goto out;
    }
    rc = template_update_attribute(publ_tmpl, ec_point_attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(ec_point_attr);
        goto out;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    d_len = EC_KEY_priv2buf(ec_key, &d);
    if (d_len == 0) {
        TRACE_ERROR("Failed to get the EC private key.\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#else
    if (!EVP_PKEY_get_bn_param(ec_pkey, OSSL_PKEY_PARAM_PRIV_KEY, &bn_d)) {
        TRACE_ERROR("EVP_PKEY_get_bn_param failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    d_len = ec_prime_len_from_nid(nid);
    d = OPENSSL_zalloc(d_len);
    if (d == NULL) {
        TRACE_ERROR("OPENSSL_zalloc failed\n");
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    BN_bn2binpad(bn_d, d, d_len);
#endif

    rc = build_attribute(CKA_VALUE, d, d_len, &value_attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("build_attribute for CKA_VALUE failed, rc=0x%lx\n", rc);
        goto out;
    }
    rc = template_update_attribute(priv_tmpl, value_attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(value_attr);
        goto out;
    }


    /* Add CKA_ECDSA_PARAMS to private template also */
    rc = build_attribute(CKA_ECDSA_PARAMS, attr->pValue, attr->ulValueLen,
                         &parms_attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("build_attribute for CKA_ECDSA_PARAMS failed, rc=0x%lx\n",
                     rc);
        goto out;
    }
    rc = template_update_attribute(priv_tmpl, parms_attr);
    if (rc != CKR_OK) {
        TRACE_ERROR("template_update_attribute failed\n");
        free(parms_attr);
        goto out;
    }

    rc = CKR_OK;

out:
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);
#if !OPENSSL_VERSION_PREREQ(3, 0)
    if (bnctx != NULL)
        BN_CTX_free(bnctx);
#else
    if (bn_d != NULL)
        BN_free(bn_d);
#endif
    if (ec_pkey != NULL)
        EVP_PKEY_free(ec_pkey);
    if (ecpoint != NULL)
        OPENSSL_free(ecpoint);
    if (enc_ecpoint != NULL)
        free(enc_ecpoint);
    if (d != NULL)
        OPENSSL_free(d);

    return rc;
}

CK_RV openssl_specific_ec_sign(STDLL_TokData_t *tokdata,  SESSION *sess,
                               CK_BYTE *in_data, CK_ULONG in_data_len,
                               CK_BYTE *out_data, CK_ULONG *out_data_len,
                               OBJECT *key_obj)
{
    EVP_PKEY *ec_key;
    ECDSA_SIG *sig = NULL;
    const BIGNUM *r, *s;
    CK_ULONG privlen, n;
    CK_RV rc = CKR_OK;
    EVP_PKEY_CTX *ctx = NULL;
    size_t siglen;
    CK_BYTE *sigbuf = NULL;
    const unsigned char *p;

    UNUSED(tokdata);
    UNUSED(sess);

    *out_data_len = 0;

    rc = openssl_make_ec_key_from_template(key_obj->template, &ec_key);
    if (rc != CKR_OK)
        return rc;

    ctx = EVP_PKEY_CTX_new(ec_key, NULL);
    if (ctx == NULL) {
        TRACE_ERROR("EVP_PKEY_CTX_new failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (EVP_PKEY_sign_init(ctx) <= 0) {
        TRACE_ERROR("EVP_PKEY_sign_init failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (EVP_PKEY_sign(ctx, NULL, &siglen, in_data, in_data_len) <= 0) {
        TRACE_ERROR("EVP_PKEY_sign failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    sigbuf = malloc(siglen);
    if (sigbuf == NULL) {
        TRACE_ERROR("malloc failed\n");
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    if (EVP_PKEY_sign(ctx, sigbuf, &siglen, in_data, in_data_len) <= 0) {
        TRACE_ERROR("EVP_PKEY_sign failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    p = sigbuf;
    sig = d2i_ECDSA_SIG(NULL, &p, siglen);
    if (sig == NULL) {
        TRACE_ERROR("d2i_ECDSA_SIG failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    ECDSA_SIG_get0(sig, &r, &s);

    privlen = ec_prime_len_from_pkey(ec_key);
    if (privlen <= 0) {
        TRACE_ERROR("ec_prime_len_from_pkey failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    /* Insert leading 0's if r or s shorter than privlen */
    n = privlen - BN_num_bytes(r);
    memset(out_data, 0, n);
    BN_bn2bin(r, &out_data[n]);

    n = privlen - BN_num_bytes(s);
    memset(out_data + privlen, 0x00, n);
    BN_bn2bin(s, &out_data[privlen + n]);

    *out_data_len = 2 * privlen;

out:
    if (sig != NULL)
        ECDSA_SIG_free(sig);
    if (ec_key != NULL)
        EVP_PKEY_free(ec_key);
    if (sigbuf != NULL)
        free(sigbuf);
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);

    return rc;
}

CK_RV openssl_specific_ec_verify(STDLL_TokData_t *tokdata,
                                 SESSION *sess,
                                 CK_BYTE *in_data,
                                 CK_ULONG in_data_len,
                                 CK_BYTE *signature,
                                 CK_ULONG signature_len, OBJECT *key_obj)
{
    EVP_PKEY *ec_key;
    CK_ULONG privlen;
    ECDSA_SIG *sig = NULL;
    BIGNUM *r = NULL, *s = NULL;
    CK_RV rc = CKR_OK;
    size_t siglen;
    CK_BYTE *sigbuf = NULL;
    EVP_PKEY_CTX *ctx = NULL;

    UNUSED(tokdata);
    UNUSED(sess);

    rc = openssl_make_ec_key_from_template(key_obj->template, &ec_key);
    if (rc != CKR_OK)
        return rc;

    privlen = ec_prime_len_from_pkey(ec_key);
    if (privlen <= 0) {
        TRACE_ERROR("ec_prime_len_from_pkey failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (signature_len < 2 * privlen) {
        TRACE_ERROR("Signature is too short\n");
        rc = CKR_SIGNATURE_LEN_RANGE;
        goto out;
    }

    sig = ECDSA_SIG_new();
    if (sig == NULL) {
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    r = BN_bin2bn(signature, privlen, NULL);
    s = BN_bin2bn(signature + privlen, privlen, NULL);
    if (r == NULL || s == NULL) {
        TRACE_ERROR("BN_bin2bn failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (!ECDSA_SIG_set0(sig, r, s)) {
        TRACE_ERROR("ECDSA_SIG_set0 failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    siglen = i2d_ECDSA_SIG(sig, &sigbuf);
    if (siglen <= 0) {
        TRACE_ERROR("i2d_ECDSA_SIG failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    ctx = EVP_PKEY_CTX_new(ec_key, NULL);
    if (ctx == NULL) {
        TRACE_ERROR("EVP_PKEY_CTX_new failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    if (EVP_PKEY_verify_init(ctx) <= 0) {
        TRACE_ERROR("EVP_PKEY_verify_init failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    rc = EVP_PKEY_verify(ctx, sigbuf, siglen, in_data, in_data_len);
    switch (rc) {
    case 0:
        rc = CKR_SIGNATURE_INVALID;
        break;
    case 1:
        rc = CKR_OK;
        break;
    default:
        rc = CKR_FUNCTION_FAILED;
        break;
    }

out:
    if (sig != NULL)
        ECDSA_SIG_free(sig);
    if (ec_key != NULL)
        EVP_PKEY_free(ec_key);
    if (sigbuf != NULL)
        OPENSSL_free(sigbuf);
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);

    return rc;
}

CK_RV openssl_specific_ecdh_pkcs_derive(STDLL_TokData_t *tokdata,
                                        CK_BYTE *priv_bytes,
                                        CK_ULONG priv_length,
                                        CK_BYTE *pub_bytes,
                                        CK_ULONG pub_length,
                                        CK_BYTE *secret_value,
                                        CK_ULONG *secret_value_len,
                                        CK_BYTE *oid, CK_ULONG oid_length)
{
#if !OPENSSL_VERSION_PREREQ(3, 0)
    EC_KEY *pub = NULL, *priv = NULL;
#else
    OSSL_PARAM_BLD *tmpl = NULL;
#endif
    EVP_PKEY *ec_pub = NULL, *ec_priv = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    size_t secret_len;
    int nid;
    CK_RV rc;

    UNUSED(tokdata);

    nid = curve_nid_from_params(oid, oid_length);
    if (nid == NID_undef) {
        TRACE_ERROR("curve not supported by OpenSSL.\n");
        rc = CKR_CURVE_NOT_SUPPORTED;
        goto out;
    }

#if !OPENSSL_VERSION_PREREQ(3, 0)
    rc = make_ec_key_from_params(oid, oid_length, &priv);
    if (rc != CKR_OK) {
        TRACE_DEVEL("make_ec_key_from_params failed\n");
        goto out;
    }
#else
    tmpl = OSSL_PARAM_BLD_new();
    if (tmpl == NULL) {
        TRACE_ERROR("OSSL_PARAM_BLD_new failed\n");
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    if (!OSSL_PARAM_BLD_push_utf8_string(tmpl, OSSL_PKEY_PARAM_GROUP_NAME,
                                         OBJ_nid2sn(nid), 0)) {
        TRACE_ERROR("OSSL_PARAM_BLD_push_utf8_string failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#endif

#if !OPENSSL_VERSION_PREREQ(3, 0)
    rc = fill_ec_key_from_privkey(priv, priv_bytes, priv_length, &ec_priv);
#else
    rc = fill_ec_key_from_privkey(tmpl, priv_bytes, priv_length, nid, &ec_priv);
#endif
    if (rc != CKR_OK) {
        TRACE_DEVEL("fill_ec_key_from_privkey failed\n");
        goto out;
    }
#if !OPENSSL_VERSION_PREREQ(3, 0)
    priv = NULL;
#else
    OSSL_PARAM_BLD_free(tmpl);
    tmpl = NULL;
#endif

#if !OPENSSL_VERSION_PREREQ(3, 0)
    rc = make_ec_key_from_params(oid, oid_length, &pub);
    if (rc != CKR_OK) {
        TRACE_DEVEL("make_ec_key_from_params failed\n");
        goto out;
    }
#else
    tmpl = OSSL_PARAM_BLD_new();
    if (tmpl == NULL) {
        TRACE_ERROR("OSSL_PARAM_BLD_new failed\n");
        rc = CKR_HOST_MEMORY;
        goto out;
    }

    if (!OSSL_PARAM_BLD_push_utf8_string(tmpl, OSSL_PKEY_PARAM_GROUP_NAME,
                                         OBJ_nid2sn(nid), 0)) {
        TRACE_ERROR("OSSL_PARAM_BLD_push_utf8_string failed\n");
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
#endif

#if !OPENSSL_VERSION_PREREQ(3, 0)
    rc = fill_ec_key_from_pubkey(pub, pub_bytes, pub_length, TRUE, nid,
                                 &ec_pub);
#else
    rc = fill_ec_key_from_pubkey(tmpl, pub_bytes, pub_length, TRUE, nid,
                                 &ec_pub);
#endif
    if (rc != CKR_OK) {
        TRACE_DEVEL("fill_ec_key_from_pubkey failed\n");
        goto out;
    }
#if !OPENSSL_VERSION_PREREQ(3, 0)
    pub = NULL;
#else
    OSSL_PARAM_BLD_free(tmpl);
    tmpl = NULL;
#endif

    ctx = EVP_PKEY_CTX_new(ec_priv, NULL);
    if (ctx == NULL) {
        TRACE_DEVEL("EVP_PKEY_CTX_new failed\n");
        goto out;
    }

    if (EVP_PKEY_derive_init(ctx) <= 0 ||
        EVP_PKEY_derive_set_peer(ctx, ec_pub) <= 0) {
        TRACE_DEVEL("EVP_PKEY_derive_init/EVP_PKEY_derive_set_peer failed\n");
        goto out;
    }

    secret_len = ec_prime_len_from_nid(nid);
    if (EVP_PKEY_derive(ctx, secret_value, &secret_len) <= 0) {
        TRACE_DEVEL("ECDH_compute_key failed\n");
        rc = CKR_FUNCTION_FAILED;
        *secret_value_len = 0;
        goto out;
    }

    *secret_value_len = secret_len;

out:
#if !OPENSSL_VERSION_PREREQ(3, 0)
    if (priv != NULL)
        EC_KEY_free(priv);
    if (pub != NULL)
        EC_KEY_free(pub);
#else
    if (tmpl != NULL)
        OSSL_PARAM_BLD_free(tmpl);
#endif
    if (ec_priv != NULL)
        EVP_PKEY_free(ec_priv);
    if (ec_pub != NULL)
        EVP_PKEY_free(ec_pub);
    if (ctx != NULL)
        EVP_PKEY_CTX_free(ctx);

    return rc;
}

#endif

static const EVP_MD *md_from_mech(CK_MECHANISM *mech)
{
    const EVP_MD *md = NULL;

    switch (mech->mechanism) {
    case CKM_SHA_1:
        md = EVP_sha1();
        break;
    case CKM_SHA224:
        md = EVP_sha224();
        break;
    case CKM_SHA256:
        md = EVP_sha256();
        break;
    case CKM_SHA384:
        md = EVP_sha384();
        break;
    case CKM_SHA512:
        md = EVP_sha512();
        break;
#ifdef NID_sha512_224WithRSAEncryption
    case CKM_SHA512_224:
        md = EVP_sha512_224();
        break;
#endif
#ifdef NID_sha512_256WithRSAEncryption
    case CKM_SHA512_256:
        md = EVP_sha512_256();
        break;
#endif
#ifdef NID_sha3_224
    case CKM_IBM_SHA3_224:
        md = EVP_sha3_224();
        break;
#endif
#ifdef NID_sha3_256
    case CKM_IBM_SHA3_256:
        md = EVP_sha3_256();
        break;
#endif
#ifdef NID_sha3_384
    case CKM_IBM_SHA3_384:
        md = EVP_sha3_384();
        break;
#endif
#ifdef NID_sha3_512
    case CKM_IBM_SHA3_512:
        md = EVP_sha3_512();
        break;
#endif
    default:
        break;
    }

    return md;
}

#if !OPENSSL_VERSION_PREREQ(3, 0)
static EVP_MD_CTX *md_ctx_from_context(DIGEST_CONTEXT *ctx)
{
    const EVP_MD *md;
    EVP_MD_CTX *md_ctx;

    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL)
        return NULL;

    md = md_from_mech(&ctx->mech);
    if (md == NULL ||
        !EVP_DigestInit_ex(md_ctx, md, NULL)) {
        TRACE_ERROR("md_from_mech or EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(md_ctx);
        return NULL;
    }

    if (ctx->context_len == 0) {
        ctx->context_len = EVP_MD_meth_get_app_datasize(EVP_MD_CTX_md(md_ctx));
        ctx->context = malloc(ctx->context_len);
        if (ctx->context == NULL) {
            TRACE_ERROR("malloc failed\n");
            EVP_MD_CTX_free(md_ctx);
            ctx->context_len = 0;
            return NULL;
        }

        /* Save context data for later use */
        memcpy(ctx->context,  EVP_MD_CTX_md_data(md_ctx), ctx->context_len);
    } else {
        if (ctx->context_len !=
                (CK_ULONG)EVP_MD_meth_get_app_datasize(EVP_MD_CTX_md(md_ctx))) {
            TRACE_ERROR("context size mismatcht\n");
            return NULL;
        }
        /* restore the MD context data */
        memcpy(EVP_MD_CTX_md_data(md_ctx), ctx->context, ctx->context_len);
    }

    return md_ctx;
}
#endif

#if OPENSSL_VERSION_PREREQ(3, 0)
static void openssl_specific_sha_free(STDLL_TokData_t *tokdata, SESSION *sess,
                                      CK_BYTE *context, CK_ULONG context_len)
{
    UNUSED(tokdata);
    UNUSED(sess);
    UNUSED(context_len);

    EVP_MD_CTX_free((EVP_MD_CTX *)context);
}
#endif

CK_RV openssl_specific_sha_init(STDLL_TokData_t *tokdata, DIGEST_CONTEXT *ctx,
                                CK_MECHANISM *mech)
{
#if !OPENSSL_VERSION_PREREQ(3, 0)
    EVP_MD_CTX *md_ctx;
#else
    const EVP_MD *md;
#endif

    UNUSED(tokdata);

    ctx->mech.ulParameterLen = mech->ulParameterLen;
    ctx->mech.mechanism = mech->mechanism;

#if !OPENSSL_VERSION_PREREQ(3, 0)
    md_ctx = md_ctx_from_context(ctx);
    if (md_ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        return CKR_HOST_MEMORY;
    }

    EVP_MD_CTX_free(md_ctx);
#else
    ctx->context_len = 1;
    ctx->context = (CK_BYTE *)EVP_MD_CTX_new();
    if (ctx->context == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        return CKR_HOST_MEMORY;
    }

    md = md_from_mech(&ctx->mech);
    if (md == NULL ||
        !EVP_DigestInit_ex((EVP_MD_CTX *)ctx->context, md, NULL)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        EVP_MD_CTX_free((EVP_MD_CTX *)ctx->context);
        return CKR_FUNCTION_FAILED;
    }

    ctx->state_unsaveable = CK_TRUE;
    ctx->context_free_func = openssl_specific_sha_free;
#endif

    return CKR_OK;
}

CK_RV openssl_specific_sha(STDLL_TokData_t *tokdata, DIGEST_CONTEXT *ctx,
                           CK_BYTE *in_data, CK_ULONG in_data_len,
                           CK_BYTE *out_data, CK_ULONG *out_data_len)
{
    unsigned int len;
    CK_RV rc = CKR_OK;
#if !OPENSSL_VERSION_PREREQ(3, 0)
    EVP_MD_CTX *md_ctx;
#endif

    UNUSED(tokdata);

    if (!ctx || !ctx->context)
        return CKR_OPERATION_NOT_INITIALIZED;

    if (!in_data || !out_data)
        return CKR_ARGUMENTS_BAD;

#if !OPENSSL_VERSION_PREREQ(3, 0)
    /* Recreate the OpenSSL MD context from the saved context */
    md_ctx = md_ctx_from_context(ctx);
    if (md_ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        return CKR_HOST_MEMORY;
    }

    if (*out_data_len < (CK_ULONG)EVP_MD_CTX_size(md_ctx))
        return CKR_BUFFER_TOO_SMALL;

    if (!EVP_DigestUpdate(md_ctx, in_data, in_data_len) ||
        !EVP_DigestFinal(md_ctx, out_data, &len)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }

    *out_data_len = len;
#else
    if (*out_data_len < (CK_ULONG)EVP_MD_CTX_size((EVP_MD_CTX *)ctx->context)) {
        TRACE_ERROR("%s\n", ock_err(ERR_BUFFER_TOO_SMALL));
        return CKR_BUFFER_TOO_SMALL;
    }

    len = *out_data_len;
    if (!EVP_DigestUpdate((EVP_MD_CTX *)ctx->context, in_data, in_data_len) ||
        !EVP_DigestFinal((EVP_MD_CTX *)ctx->context, out_data, &len)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }

    *out_data_len = len;
#endif

#if !OPENSSL_VERSION_PREREQ(3, 0)
out:
    EVP_MD_CTX_free(md_ctx);
    free(ctx->context);
#else
    EVP_MD_CTX_free((EVP_MD_CTX *)ctx->context);
#endif
    ctx->context = NULL;
    ctx->context_len = 0;
    ctx->context_free_func = NULL;

    return rc;
}

CK_RV openssl_specific_sha_update(STDLL_TokData_t *tokdata, DIGEST_CONTEXT *ctx,
                                  CK_BYTE *in_data, CK_ULONG in_data_len)
{
#if !OPENSSL_VERSION_PREREQ(3, 0)
    EVP_MD_CTX *md_ctx;
#endif

    UNUSED(tokdata);

    if (!ctx || !ctx->context)
        return CKR_OPERATION_NOT_INITIALIZED;

    if (!in_data)
        return CKR_ARGUMENTS_BAD;

#if !OPENSSL_VERSION_PREREQ(3, 0)
    /* Recreate the OpenSSL MD context from the saved context */
    md_ctx = md_ctx_from_context(ctx);
    if (md_ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        return CKR_HOST_MEMORY;
    }

    if (!EVP_DigestUpdate(md_ctx, in_data, in_data_len)) {
        EVP_MD_CTX_free(md_ctx);
        free(ctx->context);
        ctx->context = NULL;
        ctx->context_len = 0;
        ctx->context_free_func = NULL;
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }

    /* Save context data for later use */
    memcpy(ctx->context,  EVP_MD_CTX_md_data(md_ctx), ctx->context_len);

    EVP_MD_CTX_free(md_ctx);
#else
    if (!EVP_DigestUpdate((EVP_MD_CTX *)ctx->context, in_data, in_data_len)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }
#endif

    return CKR_OK;
}

CK_RV openssl_specific_sha_final(STDLL_TokData_t *tokdata, DIGEST_CONTEXT *ctx,
                                 CK_BYTE *out_data, CK_ULONG *out_data_len)
{
    unsigned int len;
    CK_RV rc = CKR_OK;
#if !OPENSSL_VERSION_PREREQ(3, 0)
    EVP_MD_CTX *md_ctx;
#endif

    UNUSED(tokdata);

    if (!ctx || !ctx->context)
        return CKR_OPERATION_NOT_INITIALIZED;

    if (!out_data)
        return CKR_ARGUMENTS_BAD;

#if !OPENSSL_VERSION_PREREQ(3, 0)
    /* Recreate the OpenSSL MD context from the saved context */
    md_ctx = md_ctx_from_context(ctx);
    if (md_ctx == NULL) {
        TRACE_ERROR("%s\n", ock_err(ERR_HOST_MEMORY));
        return CKR_HOST_MEMORY;
    }

    if (*out_data_len < (CK_ULONG)EVP_MD_CTX_size(md_ctx))
        return CKR_BUFFER_TOO_SMALL;

    if (!EVP_DigestFinal(md_ctx, out_data, &len)) {
        rc = CKR_FUNCTION_FAILED;
        goto out;
    }
    *out_data_len = len;

out:
    EVP_MD_CTX_free(md_ctx);
    free(ctx->context);
    ctx->context = NULL;
    ctx->context_len = 0;
    ctx->context_free_func = NULL;
#else
    if (*out_data_len < (CK_ULONG)EVP_MD_CTX_size((EVP_MD_CTX *)ctx->context)) {
        TRACE_ERROR("%s\n", ock_err(ERR_BUFFER_TOO_SMALL));
        return CKR_BUFFER_TOO_SMALL;
    }

    len = *out_data_len;
    if (!EVP_DigestFinal((EVP_MD_CTX *)ctx->context, out_data, &len)) {
        TRACE_ERROR("%s\n", ock_err(ERR_FUNCTION_FAILED));
        return CKR_FUNCTION_FAILED;
    }

    *out_data_len = len;

    EVP_MD_CTX_free((EVP_MD_CTX *)ctx->context);
    ctx->context = NULL;
    ctx->context_len = 0;
    ctx->context_free_func = NULL;
#endif

    return rc;
}