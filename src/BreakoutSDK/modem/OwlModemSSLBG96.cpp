#include "OwlModemSSLBG96.h"
#include <stdio.h>

OwlModemSSLBG96::OwlModemSSLBG96(OwlModemAT* atModem) : atModem_(atModem) {}

bool OwlModemSSLBG96::initContext() {
  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"sslversion\",0,3", 1 * 1000, nullptr, 0) != AT_Result_Code__OK) {
    return false;
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"seclevel\",0,2", 1 * 1000, nullptr, 0) != AT_Result_Code__OK) {
    return false;
  }

  return true;
}

bool OwlModemSSLBG96::setDeviceCert(str cert) {

  atModem_->doCommandBlocking("AT+QFDEL=\"ssl_cert.pem\"", 1 * 1000, nullptr, 0); // ignore the result, which will be error if file does not exist

  char buffer[64];
  snprintf(buffer, 64, "AT+QFUPL=\"ssl_cert.pem\",%d,100", (int) cert.len);

  if (atModem_->doCommandBlocking(buffer, 10 * 1000, nullptr, 0, cert) != AT_Result_Code__OK) {
    return false;
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"clientcert\",0,\"UFS:ssl_cert.pem\"", 1 * 1000, nullptr, 0) != AT_Result_Code__OK) {
    return false;
  }

  return true;
}

bool OwlModemSSLBG96::setDevicePkey(str pkey) {
  atModem_->doCommandBlocking("AT+QFDEL=\"ssl_pkey.pem\"", 1 * 1000, nullptr, 0); // ignore the result, which will be error if file does not exist

  char buffer[64];
  snprintf(buffer, 64, "AT+QFUPL=\"ssl_pkey.pem\",%d,100", (int) pkey.len);

  if (atModem_->doCommandBlocking(buffer, 10 * 1000, nullptr, 0, pkey) != AT_Result_Code__OK) {
    return false;
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"clientkey\",0,\"UFS:ssl_pkey.pem\"", 1 * 1000, nullptr, 0) != AT_Result_Code__OK) {
    return false;
  }

  return true;
}

bool OwlModemSSLBG96::setServerCA(str ca) {
  atModem_->doCommandBlocking("AT+QFDEL=\"ssl_cacert.pem\"", 1 * 1000, nullptr, 0); // ignore the result, which will be error if file does not exist

  char buffer[64];
  snprintf(buffer, 64, "AT+QFUPL=\"ssl_cacert.pem\",%d,100", (int) ca.len);

  if (atModem_->doCommandBlocking(buffer, 10 * 1000, nullptr, 0, ca) != AT_Result_Code__OK) {
    return false;
  }

  if (atModem_->doCommandBlocking("AT+QSSLCFG=\"cacert\",0,\"UFS:ssl_cacert.pem\"", 1 * 1000, nullptr, 0) != AT_Result_Code__OK) {
    return false;
  }

  return true;
}
