#include "OwlModemSSLBG96.h"
#include <stdio.h>

OwlModemSSLBG96::OwlModemSSLBG96(OwlModemAT* atModem) : atModem_(atModem) {
}

bool OwlModemSSLBG96::initContext() {
  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"sslversion\",0,3", 1 * 1000, nullptr) != AT_Result_Code__OK) {
    return false;
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"seclevel\",0,2", 1 * 1000, nullptr) != AT_Result_Code__OK) {
    return false;
  }

  return true;
}

static str s_qfopen = STRDECL("+QFOPEN: ");

bool OwlModemSSLBG96::setDeviceCert(str cert, bool force) {
  bool write_cert = true;

  if (!force) {
    if (atModem_->doCommandBlocking("AT+QFOPEN=\"ssl_cert.pem\",2", 1 * 1000, &ssl_response) == AT_Result_Code__OK) {
      // Just check if the file exists. A better solution would be to read out the file and compare, or use QFCRC, which
      // is only present in the newest firmware revisions
      OwlModemAT::filterResponse(s_qfopen, ssl_response, &ssl_response);
      long int fdesc = str_to_long_int(ssl_response, 10);
      char closebuf[32];
      snprintf(closebuf, 32, "AT+QFCLOSE=%d", (int)fdesc);
      if (atModem_->doCommandBlocking(closebuf, 1 * 1000, &ssl_response) != AT_Result_Code__OK) {
        LOG(L_WARN, "Couldn't close certificate file descriptor");
      }
      write_cert = false;
    }
  }

  if (write_cert) {
    atModem_->doCommandBlocking("AT+QFDEL=\"ssl_cert.pem\"", 1 * 1000,
                                nullptr);  // ignore the result, which will be error if file does not exist

    char buffer[64];
    snprintf(buffer, 64, "AT+QFUPL=\"ssl_cert.pem\",%d,100", (int)cert.len);

    if (atModem_->doCommandBlocking(buffer, 10 * 1000, nullptr, cert) != AT_Result_Code__OK) {
      return false;
    }
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"clientcert\",0,\"UFS:ssl_cert.pem\"", 1 * 1000, nullptr) !=
      AT_Result_Code__OK) {
    return false;
  }

  return true;
}

bool OwlModemSSLBG96::setDevicePkey(str pkey, bool force) {
  bool write_pkey = true;

  if (!force) {
    if (atModem_->doCommandBlocking("AT+QFOPEN=\"ssl_pkey.pem\",2", 1 * 1000, &ssl_response) == AT_Result_Code__OK) {
      // Just check if the file exists. A better solution would be to read out the file and compare, or use QFCRC, which
      // is only present in the newest firmware revisions
      OwlModemAT::filterResponse(s_qfopen, ssl_response, &ssl_response);
      long int fdesc = str_to_long_int(ssl_response, 10);
      char closebuf[32];
      snprintf(closebuf, 32, "AT+QFCLOSE=%d", (int)fdesc);
      if (atModem_->doCommandBlocking(closebuf, 1 * 1000, &ssl_response) != AT_Result_Code__OK) {
        LOG(L_WARN, "Couldn't close key file descriptor");
      }
      write_pkey = false;
    }
  }

  if (write_pkey) {
    atModem_->doCommandBlocking("AT+QFDEL=\"ssl_pkey.pem\"", 1 * 1000,
                                nullptr);  // ignore the result, which will be error if file does not exist

    char buffer[64];
    snprintf(buffer, 64, "AT+QFUPL=\"ssl_pkey.pem\",%d,100", (int)pkey.len);

    if (atModem_->doCommandBlocking(buffer, 10 * 1000, nullptr, pkey) != AT_Result_Code__OK) {
      return false;
    }
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"clientkey\",0,\"UFS:ssl_pkey.pem\"", 1 * 1000, nullptr) !=
      AT_Result_Code__OK) {
    return false;
  }

  return true;
}

bool OwlModemSSLBG96::setServerCA(str ca, bool force) {
  bool write_ca = true;

  if (!force) {
    if (atModem_->doCommandBlocking("AT+QFOPEN=\"ssl_cacert.pem\",2", 1 * 1000, &ssl_response) == AT_Result_Code__OK) {
      // Just check if the file exists. A better solution would be to read out the file and compare, or use QFCRC, which
      // is only present in the newest firmware revisions
      OwlModemAT::filterResponse(s_qfopen, ssl_response, &ssl_response);
      long int fdesc = str_to_long_int(ssl_response, 10);
      char closebuf[32];
      snprintf(closebuf, 32, "AT+QFCLOSE=%d", (int)fdesc);
      if (atModem_->doCommandBlocking(closebuf, 1 * 1000, &ssl_response) != AT_Result_Code__OK) {
        LOG(L_WARN, "Couldn't close CA certificate file descriptor");
      }
      write_ca = false;
    }
  }

  if (write_ca) {
    atModem_->doCommandBlocking("AT+QFDEL=\"ssl_cacert.pem\"", 1 * 1000,
                                nullptr);  // ignore the result, which will be error if file does not exist

    char buffer[64];
    snprintf(buffer, 64, "AT+QFUPL=\"ssl_cacert.pem\",%d,100", (int)ca.len);

    if (atModem_->doCommandBlocking(buffer, 10 * 1000, nullptr, ca) != AT_Result_Code__OK) {
      return false;
    }
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"cacert\",0,\"UFS:ssl_cacert.pem\"", 1 * 1000, nullptr) !=
      AT_Result_Code__OK) {
    return false;
  }

  return true;
}
