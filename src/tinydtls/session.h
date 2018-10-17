/*******************************************************************************
 *
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Olaf Bergmann (TZI) and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v. 1.0 which accompanies this distribution.
 *
 * The Eclipse Public License is available at http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Olaf Bergmann  - initial API and implementation
 *
 *******************************************************************************/

#ifndef _DTLS_SESSION_H_
#define _DTLS_SESSION_H_

#include <string.h>

#include "tinydtls.h"
#include "global.h"

#ifdef WITH_CONTIKI
#include "ip/uip.h"
typedef struct {
  unsigned char size;
  uip_ipaddr_t addr;
  unsigned short port;
  int ifindex;
} session_t;
 /* TODO: Add support for RIOT over sockets  */
#elif defined(WITH_RIOT_GNRC)
#include "net/ipv6/addr.h"
typedef struct {
  unsigned char size;
  ipv6_addr_t addr;
  unsigned short port;
  int ifindex;
} session_t;
#elif defined(WITH_ARDUINO)

#include <stdint.h>

typedef uint8_t socklen_t;

typedef enum {
  IP_Address__empty = 0,
  IP_Address__IPv4  = 4,
  IP_Address__IPv6  = 16,
} ip_address_len_e;

typedef union {
  uint8_t ipv4[4];
  uint8_t ipv6[16];
} ip_address_u;

typedef struct {
  socklen_t size; /**< size of addr - 4 or 16, same as ip_address_len_e bytes*/
  ip_address_u addr;
  unsigned short port;
  int ifindex;
} session_t;

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct {
  socklen_t size;		/**< size of addr */
  union {
    struct sockaddr     sa;
    struct sockaddr_storage st;
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;
  } addr;
  uint8_t ifindex;
} session_t;
#endif /* WITH_CONTIKI */

/** 
 * Resets the given session_t object @p sess to its default
 * values.  In particular, the member rlen must be initialized to the
 * available size for storing addresses.
 * 
 * @param sess The session_t object to initialize.
 */
void dtls_session_init(session_t *sess);

#if !(defined(WITH_CONTIKI)) && !(defined(RIOT_VERSION)) && !(defined(WITH_ARDUINO))
/**
 * Creates a new ::session_t for the given address.
 *
 * @param addr Address which should be stored in the ::session_t.
 * @param addrlen Length of the @p addr.
 * @return The new session or @c NULL on error.
 */
session_t* dtls_new_session(struct sockaddr *addr, socklen_t addrlen);

/**
 * Frees memory allocated for a session using ::dtls_new_session.
 *
 * @param sess Pointer to a session for which allocated memory should be
 *     freed.
 */
void dtls_free_session(session_t *sess);

/**
 * Extracts the address of the given ::session_t.
 *
 * @param sess Session to extract address for.
 * @param addrlen Pointer to memory location where the address
 *     length should be stored.
 * @return The address or @c NULL if @p sess was @c NULL.
 */
struct sockaddr *dtls_session_addr(session_t *sess, socklen_t *addrlen);
#elif defined(WITH_ARDUINO)
/**
 * Creates a new ::session_t for the given address.
 *
 * @param addr Address which should be stored in the ::session_t.
 * @param addrlen Length of the @p addr.
 * @return The new session or @c NULL on error.
 */
session_t *dtls_new_session(ip_address_u *addr, socklen_t addrlen);

/**
 * Frees memory allocated for a session using ::dtls_new_session.
 *
 * @param sess Pointer to a session for which allocated memory should be
 *     freed.
 */
void dtls_free_session(session_t *sess);

/**
 * Extracts the address of the given ::session_t.
 *
 * @param sess Session to extract address for.
 * @param addrlen Pointer to memory location where the address
 *     length should be stored.
 * @return The address or @c NULL if @p sess was @c NULL.
 */
ip_address_u *dtls_session_addr(session_t *sess, socklen_t *addrlen);
#endif


/**
 * Compares the given session objects. This function returns @c 0
 * when @p a and @p b differ, @c 1 otherwise.
 */
int dtls_session_equals(const session_t *a, const session_t *b);

#endif /* _DTLS_SESSION_H_ */
