#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "BreakoutSDK/modem/OwlModemAT.h"

std::vector<std::string> received_strings;

void spinProcessLineTestpoint(str line) {
  received_strings.push_back(std::string(line.s, line.len));
}

class TestSerial : public IOwlSerial {
 public:
  int32_t available() {
    return mt_to_te.length();
  }
  int32_t read(uint8_t* buf, uint32_t count) {
    uint32_t to_read = (count > mt_to_te.length()) ? mt_to_te.length() : count;

    memcpy(buf, mt_to_te.c_str(), to_read);
    mt_to_te = mt_to_te.substr(to_read);

    return to_read;
  }

  int32_t write(const uint8_t* buf, uint32_t count) {
    std::string data = std::string((char*)buf, count);

    te_to_mt += data;

    return count;
  }
  std::string mt_to_te;
  std::string te_to_mt;
};

TEST_CASE("OwlModemAT breaks input into lines correctly", "[linesplit]") {
  SECTION("simple case") {
    INFO("Testing simple case");
    TestSerial serial;
    OwlModemAT modem(&serial);

    received_strings.clear();
    serial.mt_to_te += "\r\nLINE0\r\n\r\nLINE1\r\n\r\nLINE2\r\n";

    modem.spin();

    REQUIRE(received_strings == std::vector<std::string>({"LINE0", "LINE1", "LINE2"}));
  }

  SECTION("in the middle") {
    INFO("Testing starting in the middle of stream");
    TestSerial serial;
    OwlModemAT modem(&serial);

    received_strings.clear();
    serial.mt_to_te += "LINE0\r\n\r\nLINE1\r\n\r\nLINE2\r\n";

    modem.spin();

    REQUIRE(received_strings ==
            std::vector<std::string>({"LINE1", "LINE2"}));  // LINE0 lacks leading \r\n and should be ignored
  }

  SECTION("fuzzy input") {
    INFO("Testing input with corrupted delimeters");
    TestSerial serial;
    OwlModemAT modem(&serial);

    received_strings.clear();
    serial.mt_to_te += "\r\nLINE0\n\nLINE1\r\n\r\nLINE2\r\n\rLINE3\r\n\r\nLINE4\r\n";

    modem.spin();

    // lines 0, 1 and 3 can be lost or corrupted, lines 2 and 4 should be present
    bool line_2_found = false;
    bool line_4_found = false;
    for (auto& line : received_strings) {
      if (line == "LINE2") {
        line_2_found = true;
      }

      if (line == "LINE4") {
        line_4_found = true;
      }
    }

    REQUIRE(line_2_found);
    REQUIRE(line_4_found);
  }
}

std::vector<std::pair<std::string, std::string>> test_urcs;

bool test_urc_handler(str urc, str data, void* priv) {
  if (std::string(urc.s, urc.len) == "+CPIN") {
    test_urcs.push_back({std::string(urc.s, urc.len), std::string(data.s, data.len)});
    return true;
  } else {
    return false;
  }
}

TEST_CASE("OwlModemAT calls URC handlers", "[urc]") {
  INFO("Testing URC handlers");

  TestSerial serial;
  OwlModemAT modem(&serial);

  test_urcs.clear();

  REQUIRE(modem.registerUrcHandler("Test", test_urc_handler, nullptr));

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::idle);

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::idle);

  REQUIRE(test_urcs.size() == 0);

  serial.mt_to_te += "\r\nRDY\r\n\r\n+CPIN: READY\r\n\r\nWild string\r\n";

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(test_urcs.size() == 1);
  REQUIRE(test_urcs[0].first == "+CPIN");
  REQUIRE(test_urcs[0].second == "READY");

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::idle);
}

TEST_CASE("OwlModemAT processes simple commands correctly", "[command]") {
  INFO("Testing simple command");
  TestSerial serial;
  OwlModemAT modem(&serial);

  str command = STRDECL("AT+COPS?");
  REQUIRE(modem.startATCommand(command, 1000));
  REQUIRE(serial.te_to_mt == "AT+COPS?\r\n");

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_result);

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_result);

  serial.mt_to_te += "\r\n+COPS: 1\r\n\r\nOK\r\n";

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::response_ready);
  str response;
  REQUIRE(modem.getLastCommandResponse(&response) == AT_Result_Code__OK);

  REQUIRE(std::string(response.s, response.len) == "+COPS: 1\n");
}

TEST_CASE("OwlModemAT processes URC while command is running", "[command-urc]") {
  INFO("Testing command with URC");

  TestSerial serial;
  OwlModemAT modem(&serial);

  test_urcs.clear();

  REQUIRE(modem.registerUrcHandler("Test", test_urc_handler, nullptr));

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::idle);

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  str command = STRDECL("AT+COPS?");
  REQUIRE(modem.startATCommand(command, 1000));
  REQUIRE(serial.te_to_mt == "AT+COPS?\r\n");

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_result);

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_result);

  REQUIRE(test_urcs.size() == 0);

  serial.mt_to_te += "\r\n+COPS: 1\r\n\r\n+CPIN: READY\r\n\r\nOK\r\n";

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(test_urcs.size() == 1);
  REQUIRE(test_urcs[0].first == "+CPIN");
  REQUIRE(test_urcs[0].second == "READY");

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::response_ready);

  str response;
  REQUIRE(modem.getLastCommandResponse(&response) == AT_Result_Code__OK);

  REQUIRE(std::string(response.s, response.len) == "+COPS: 1\n");
}

TEST_CASE("OwlModemAT processes commands with data", "[command-data]") {
  INFO("Testing command with data");

  TestSerial serial;
  OwlModemAT modem(&serial);
  std::string data_string =
      "For the understanding, like the eye, judging of objects only by its own sight, cannot but be pleased with what "
      "it discovers, having less regret for what has escaped it, because it is unknown. Thus he who has raised himself "
      "above the alms-basket, and, not content to live lazily on scraps of begged opinions, sets his own thoughts on "
      "work, to find and follow truth, will (whatever he lights on) not miss the hunter’s satisfaction; every moment "
      "of his pursuit will reward his pains with some delight; and he will have reason to think his time not ill "
      "spent, even when he cannot much boast of any great acquisition.";
  std::string command_string = "AT_QFUPL=\"file\"," + std::to_string(data_string.length());
  str command                = {.s = (char*)command_string.c_str(), .len = command_string.length()};
  str data                   = {.s = (char*)data_string.c_str(), .len = data_string.length()};

  REQUIRE(modem.startATCommand(command, 1000, data));
  REQUIRE(serial.te_to_mt == (command_string + "\r\n"));

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_prompt);

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_prompt);

  serial.te_to_mt.clear();
  serial.mt_to_te += "\r\nCONNECT\r\n";

  for (int i = 0; i < 50; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::wait_result);
  REQUIRE(serial.te_to_mt == data_string);
  serial.mt_to_te += "\r\nOK\r\n";

  for (int i = 0; i < 5; i++) {
    // spin for a while
    modem.spin();
  }

  REQUIRE(modem.getModemState() == OwlModemAT::modem_state_t::response_ready);
  str response;
  REQUIRE(modem.getLastCommandResponse(&response) == AT_Result_Code__OK);

  REQUIRE(std::string(response.s, response.len) == "");
}
