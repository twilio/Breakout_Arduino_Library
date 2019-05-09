#include <board.h>
#include <BreakoutSDK/modem/OwlModemBG96.h>
#include <stdio.h>

// https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor
#include <DHT.h>

// Redefine tls_cert, tls_pkey and tls_cacert with your own device certificate,
//   device private key and server CA certificate respectively

str tls_cert = STRDECL(
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEfzCCAmcCCQDUuQBXuzl2njANBgkqhkiG9w0BAQsFADCBhTELMAkGA1UEBhMC\n"
    "SVMxEDAOBgNVBAgMB0ljZWxhbmQxEjAQBgNVBAcMCVJleWtqYXZpazESMBAGA1UE\n"
    "CgwJTUlBUyBUZXN0MScwJQYDVQQLDB5DZXJ0aWZpY2F0ZSBpc3N1aW5nIGRlcGFy\n"
    "dG1lbnQxEzARBgNVBAMMCmV4YW1wbGUuaXMwHhcNMTkwNTA4MTMzMzQ5WhcNMjQw\n"
    "NTA2MTMzMzQ5WjB9MQswCQYDVQQGEwJJUzEQMA4GA1UECAwHSWNlbGFuZDESMBAG\n"
    "A1UEBwwJUmV5a2phdmlrMRIwEAYDVQQKDAlNSUFTIFRlc3QxFjAUBgNVBAsMDVFB\n"
    "IERlcGFydG1lbnQxHDAaBgNVBAMME1plcGh5ciBRdWVjdGVsIEJHOTYwggEiMA0G\n"
    "CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCjJ8AclFX+hA6QCy/gGxUYCgvwUmio\n"
    "Fy3XgHd6wBnX15kqEo0QFf+zQ2Zaql/GVwhJ2rL5/Z2lVUzdnd+CjanDXNnYmlMd\n"
    "qRduqV09PbJq4VGmr0iSlp1c6y6I/lbLU3qkPthUWzDBGPPMRlEHHoO48hH+4cZy\n"
    "oUKctfvkigPKDYTblRJW2+gN9Ptua+urMvNkxxsFfdZpwe9s4/8DI6ouBAlpvgfY\n"
    "86S1f3f2++eL0nmhanJOAkbaVDs2DcGWDKFUdUP1c832dRpkh9ny4yK2F3s2RRRa\n"
    "nhNQEQp+H/WDGMXIKZcLvrGeDXAwbz5C8Xh2xvSb0VGznIHXQSqf4KlbAgMBAAEw\n"
    "DQYJKoZIhvcNAQELBQADggIBANFn6su7SZBxDa+mMrN3tytQBJRVEwuBbs1Ri5g9\n"
    "UBcMxi1VZX3ievvg3KMb+Isfu+mIVkR9Iig2mNAByjztR7ejF0MpkDOYYPHUx1lK\n"
    "T29XpcP5FrN3J+0IPidtD5kjR7Wf0vVeuYOFrDNA/bT7ltA4YAQS5lvrb7V7pDjQ\n"
    "XX1KNg39olShTxvZX7/+hUavV4ExGLuYq7brVJktQ7u8k5Nao5FZhoeDr0PsLoPF\n"
    "n4sbh96oSAcmcAXtpx5ZB33xGJvhHREDZPL9I1FeB6MgoPhivExP9J7Ft8IT+jJu\n"
    "jsZDywOwOB6zsKtfQFMlChrYICyO91uk+f63toZzi5FtTN7XcUNDb859cC2AilBH\n"
    "ZsZQacHPDV1ZgnGjt3p/Zz5djG3XlO5QjHYHzxZ4u9O/0f1QwR+658VXjqTltv9h\n"
    "AGWiuXcWC3V2JuTFyDuMMbFRFUYYyPODdDMUC2nFDay5fH12A+TxqDYNW+N2VMZM\n"
    "kquIl2Nn7ZzKvq8VlD7KMICL3o+U9rJT4D8qjIGIFlCNn0ni3EKOS5rIwixPPbv4\n"
    "cRI1zELUMD4VKs3E/wI90eN1o/E8PHSdRbOfNyHP6SGxJY+1KPvlU+k6d6zNZ/tM\n"
    "8G9F7eD7B8spskplmwtaTd6a4EmV3wj+eh9wVL9W0koSoAahTYlmdBUYffX8QX2D\n"
    "KbFK\n"
    "-----END CERTIFICATE-----\n");

str tls_pkey = STRDECL(
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEowIBAAKCAQEAoyfAHJRV/oQOkAsv4BsVGAoL8FJoqBct14B3esAZ19eZKhKN\n"
    "EBX/s0NmWqpfxlcISdqy+f2dpVVM3Z3fgo2pw1zZ2JpTHakXbqldPT2yauFRpq9I\n"
    "kpadXOsuiP5Wy1N6pD7YVFswwRjzzEZRBx6DuPIR/uHGcqFCnLX75IoDyg2E25US\n"
    "VtvoDfT7bmvrqzLzZMcbBX3WacHvbOP/AyOqLgQJab4H2POktX939vvni9J5oWpy\n"
    "TgJG2lQ7Ng3BlgyhVHVD9XPN9nUaZIfZ8uMithd7NkUUWp4TUBEKfh/1gxjFyCmX\n"
    "C76xng1wMG8+QvF4dsb0m9FRs5yB10Eqn+CpWwIDAQABAoIBAHkf86HEBiuTwnPx\n"
    "ujM4J+rW2pIpvAS+YIwSlaENqKHzL4RqjUsZ6eEd8ojw33WR+1dJA4JQZI7vysbk\n"
    "g6CapyOgdSI3P+lPNVQ0bEIg4aozaLjEfK5HHsBy4PNXTvxYFz6EYXoO0R0p9yT6\n"
    "NwGAB+5v2ChPWVKUoa1R9ZVvWOzQ+OgQDQVfAlRv2CvvIvrsR3tgfj7zid4VRXP/\n"
    "8s5xq5m8O8Rs08LBD2wMDP6OVcE8GAspGCX7304RCjUjdPQTPADAc15oug3J2whW\n"
    "z74CKzAZwgUyLhibu88t6frD2pFkUsc+TYaAD1M7bqVYQ3uFN2Ca/CDmN+ZAyD2J\n"
    "ZaMuQAECgYEA01Z7wHNJtGgFk4Q+31+U9UNHW60rWvgpZEyPv815q4fuQQ4qziSS\n"
    "+CsaAsb/zChuvMWChFfrdqWcditLTsEflVrRlOhLHQ6bSvpzZJkHnRg9oR99TFlO\n"
    "XFjhrJXzBjgTF0il4TiO9DutmkCVc9ngZkd94+0qCfV6A2sbQFpe2VsCgYEAxaKP\n"
    "H1PdbHzEZN/rXgzsXjXUg7ZkGAhAdJmKGdF9iJoGufgK7/Wo56eUhd/KVIguB9oU\n"
    "V2jTDA1b47IHau5kLjUdV8b6iJEz2olVu+JgZ1mntTMpIBoYOSTzxyLgE39D/+iV\n"
    "dYCcyHKudK/oDk7MAIGrmfLXruO1tkdEEZVecAECgYAxp3IwB2Zb0szsmffDt8th\n"
    "zMrpSUiUeRYQkMR9hiN+H9PkyRVZldJKKKZV3LehGibah3Vg7t9N4x9dzFJHUKzB\n"
    "BLOVTvbG/vWRqkKOcj4NtPJV9vYTiDAXFnL/f8O3xFkH8XO39PfxfkwNn/r9W0WU\n"
    "Alwbv09PQ7PFNdcTSahbXQKBgCWSWNkgzWhxc7ilpQ41ML5cR3FevDqhXveLtOhh\n"
    "nhbZCUxTbmjd7+VSQ3cL62AUn4OYnuNbJzwUUhLAZo6akWsDZ/em+Tv7Nrtl/mmA\n"
    "iMk9DxfwiPH0ZASBFOMXqzepqxi8c6Vp9ORagPXn9xq5Oikifaf/tacm3QWxGKyr\n"
    "E9ABAoGBANE3hrdt6vzvldbmXfojPaKjZbgXwBqwjgMwQjOfLz8HKKBoAox62Efv\n"
    "BAacWFvH6tdQn9EyxWB2yRz2PsK5LUcaJ3cNUqwLoarQH5VaPIV4lWtcoFD0hgx/\n"
    "t1nBBVua4V2ERktQjqeDDBVd4PqEwDFKZrC9uXIfABHH2HNyHTBs\n"
    "-----END RSA PRIVATE KEY-----\n");

str tls_cacert = STRDECL(
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF4jCCA8qgAwIBAgIJANSXMXyoe8mxMA0GCSqGSIb3DQEBCwUAMIGFMQswCQYD\n"
    "VQQGEwJJUzEQMA4GA1UECAwHSWNlbGFuZDESMBAGA1UEBwwJUmV5a2phdmlrMRIw\n"
    "EAYDVQQKDAlNSUFTIFRlc3QxJzAlBgNVBAsMHkNlcnRpZmljYXRlIGlzc3Vpbmcg\n"
    "ZGVwYXJ0bWVudDETMBEGA1UEAwwKZXhhbXBsZS5pczAeFw0xOTA1MDgxMzMzNDla\n"
    "Fw0yNDA1MDYxMzMzNDlaMIGFMQswCQYDVQQGEwJJUzEQMA4GA1UECAwHSWNlbGFu\n"
    "ZDESMBAGA1UEBwwJUmV5a2phdmlrMRIwEAYDVQQKDAlNSUFTIFRlc3QxJzAlBgNV\n"
    "BAsMHkNlcnRpZmljYXRlIGlzc3VpbmcgZGVwYXJ0bWVudDETMBEGA1UEAwwKZXhh\n"
    "bXBsZS5pczCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBANGX3HlYuRl9\n"
    "mhg9B7HgspPAkxQfwuCC8S+FheKLh5H91uw2gqmQYnz8wQITFQnWabQrd2f/R2ni\n"
    "X2nFO3FBddvfOhVead7/3NsDz14jWqpq1ycdHApC8kQKIvrdOtd9zlkAWEMJ0OMb\n"
    "Y4d4TS/dYMjsFuvtyRPakTLq2WwuZ5JDfitfQYile/c8N5OPUNSA8gdKW7fLj8z0\n"
    "mitMrzPjcrkotrcMfla66RgIDiNB7+kwOHuLVGkVhCChRH/nzmISelY/fGOUnDWa\n"
    "jwjih6ECwQkCXb280ek+FWOoOP8wKXUIe9lfRF3rSf8ha5v9URoDdg/oTMAtpIrY\n"
    "+7V2hTrKeTPH9CNj6AxfIBrnTOgwsatYqHRjnFdWwLnpFu+4qRxL0FmqUrm9dN8T\n"
    "5rnGGrquT2KAFwgMGktyHR9uK1N+xHA8o4KJOreayjRSBN4//qUApYlOUC5fOZ2M\n"
    "o0D5AS9Qgq8RkP5maQxhKWiRJHfsXvVru3x2Jq+dRvBXFLeBTjDTIALOvp5UvLjv\n"
    "+jMIR/KlbLcETXMD4OvEQ0UPl1p+57ePAg4oh8M3hOTlPiYnOhFIoSxhKwqB+3NP\n"
    "a+X7kVO/4OmWPVOi6rUoGaqIhrOcoNrXKp3eQY56OjJe1G/A8UqNo71kzvazKz16\n"
    "eVq8ggNNEnXuaXAPnBwtexuer2uMfpjXAgMBAAGjUzBRMB0GA1UdDgQWBBRpIayE\n"
    "5koFTi+kcdzrlt63MaEEojAfBgNVHSMEGDAWgBRpIayE5koFTi+kcdzrlt63MaEE\n"
    "ojAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4ICAQDAOjfUsGNNpxDu\n"
    "IoC8fah1g7wry1ndkJCCl+NkCHWvwGC4jMRXD7JOLV0XSu1481F3N+J+OzbXGx6A\n"
    "Q73NPErVDuEnXNd6ckFvH2g/j0XxV94XVlxxf6x5Dlex9EL6/JBPg9pVenqAqgeU\n"
    "m2Te8INPzoMUXYaRqAqGsOunJkq0cFX2DIvy3sFgtyWi9NSx/nfpsqqUGqfpjMi8\n"
    "2hoQbFBrZ9QhiLOPkyaSpp50zDSHvL6+4rRNO2cTPuzCx3QkLPiokz1r9W04XL1Z\n"
    "NKDVKS7x4OD8TlfZllWXI6r0hlUew/i6cTfOzPLR9f9IETI/sB8axmcVkMJXfIoz\n"
    "xuBNluvFKZdjnoke8mFZ2mnJlOuq/k6JOzRdhDhCRDxYZocx2BBAIuRZLU3+++BM\n"
    "SgKs8dEFCiIHnEBTT2l/5vaMp1yL1Q8m334f+1BJig0YfTrbMD7ETBEFxtEkdO7y\n"
    "z2PV9mXojirG/Kirp1NL3R6oADw2W4ZA8rTKeaoe3X/1NnVfxH9i80FUKGkB6k1s\n"
    "RrQcvz9ffiwHPWkTclg+e2+k/wvZSqxuorN5s1ISdh9t6xGlfTFIt1cbLAw83twd\n"
    "P0nWD523hY9SLw9psaOENP7u264X8rsvS6wOiPyAgQ2rlV9QJD8a2V6nfaNWdUFC\n"
    "OEJKzGK1LSQ7ZS/dmgDfWxozY9lLCw==\n"
    "-----END CERTIFICATE-----\n");

#define FORCE_OVERRIDE_CREDENTIALS false

#define MQTT_BROKER_HOST "10.64.95.217"
#define MQTT_BROKER_PORT 11111
#define MQTT_CLIENT_ID "arduino-bg96-gps"
#define MQTT_LOGIN NULL
#define MQTT_PASSWORD NULL

OwlModemBG96 *bg96_modem = nullptr;

#define LOOP_INTERVAL (200)
#define SEND_INTERVAL (5 * 1000)

#define SENSOR_PIN (D38)
#define DHTTYPE DHT11  // DHT 11

DHT dht(SENSOR_PIN, DHTTYPE);

bool sleep = false;

static void device_state_callback(str topic, str data) {
  if (!str_equal_char(topic, "state")) {
    LOG(L_WARN, "Unknown topic: %.*s\r\n", topic.len, topic.s);
    return;
  }

  if (str_equal_char(data, "sleep")) {
    if (sleep) {
      LOG(L_INFO, "Already sleeping\r\n");
    } else {
      LOG(L_INFO, "Going to sleep\r\n");
      sleep = true;
    }
    return;
  }

  if (str_equal_char(data, "wakeup")) {
    if (!sleep) {
      LOG(L_INFO, "Already awake\r\n");
    } else {
      LOG(L_INFO, "Waking up\r\n");
      sleep = false;
    }
    return;
  }

  LOG(L_WARN, "Unknown state: %.*s\r\n", data.len, data.s);
}

void fail() {
  LOG(L_ERR, "Initialization failed, entering bypass mode. Reset the board to start again.\r\n");

  while (true) {
    if (SerialDebugPort.available()) {
      int bt = SerialDebugPort.read();
      SerialGrove.write(bt);
    }

    if (SerialGrove.available()) {
      int bt = SerialGrove.read();
      SerialDebugPort.write(bt);
    }
  }
}

void setup() {
  // Feel free to change the log verbosity. E.g. from most critical to most verbose:
  //   - errors: L_ALERT, L_CRIT, L_ERR, L_ISSUE
  //   - warnings: L_WARN, L_NOTICE
  //   - information & debug: L_INFO, L_DB, L_DBG, L_MEM
  // When logging, the additional L_CLI level ensure that the output will always be visible, no matter the set level.
  owl_log_set_level(L_INFO);
  LOG(L_WARN, "Arduino setup() starting up\r\n");

  bg96_modem = new OwlModemBG96(&SerialGrove);

  LOG(L_WARN, "Powering on module...");
  if (!bg96_modem->powerOn()) {
    LOG(L_WARN, "... error powering on.\r\n");
    fail();
  }
  LOG(L_WARN, "... done powering on.\r\n");

  LOG(L_WARN, "Initializing the module and registering on the network...");
  if (!bg96_modem->initModem()) {
    LOG(L_WARN, "... error initializing.\r\n");
    fail();
  }
  LOG(L_WARN, "... done initializing.\r\n");

  LOG(L_WARN, "Waiting for network registration...");
  if (!bg96_modem->waitForNetworkRegistration(30000)) {
    LOG(L_WARN, "... error registering on the network.\r\n");
    fail();
  }
  LOG(L_WARN, "... done waiting.\r\n");

  if (!bg96_modem->ssl.initContext()) {
    LOG(L_WARN, "Failed to initialize TLS context\r\n");
    fail();
  }

  if (!bg96_modem->ssl.setDeviceCert(tls_cert, FORCE_OVERRIDE_CREDENTIALS)) {
    LOG(L_WARN, "Failed to initialize TLS certificate\r\n");
    fail();
  }

  if (!bg96_modem->ssl.setDevicePkey(tls_pkey, FORCE_OVERRIDE_CREDENTIALS)) {
    LOG(L_WARN, "Failed to initialize TLS private key\r\n");
    fail();
  }

  if (!bg96_modem->ssl.setServerCA(tls_cacert, FORCE_OVERRIDE_CREDENTIALS)) {
    LOG(L_WARN, "Failed to initialize TLS peer CA\r\n");
    fail();
  }

  bg96_modem->mqtt.useTLS(true);

  if (!bg96_modem->mqtt.openConnection(MQTT_BROKER_HOST, MQTT_BROKER_PORT)) {
    LOG(L_WARN, "Failed to open connection to MQTT broker\r\n");
    fail();
  }

  if (!bg96_modem->mqtt.login(MQTT_CLIENT_ID, MQTT_LOGIN, MQTT_PASSWORD)) {
    LOG(L_WARN, "Failed to login to MQTT broker\r\n");
    fail();
  }

  bg96_modem->mqtt.setMessageCallback(device_state_callback);
  if (!bg96_modem->mqtt.subscribe("state", 1)) {
    LOG(L_WARN, "Failed to subscribe to \"state\" topic\r\n");
    fail();
  }

  LOG(L_WARN, "Arduino loop() starting up\r\n");
}

void loop() {
  static unsigned long last_send = 0;

  if ((last_send == 0) || (millis() - last_send >= SEND_INTERVAL)) {
    last_send = millis();

    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();

    char commandText[512];
    snprintf(commandText, 512, "Current humidity: [%4.2f] and current temp [%4.2f]", humidity, temperature);
    str commandStr = {commandText, strlen(commandText)};
    bg96_modem->mqtt.publish("temperature", commandStr);
  }

  bg96_modem->AT.spin();

  delay(LOOP_INTERVAL);
}
