#include "NetworkClient.hpp"
#include <iostream>

static void usage(char *program_name) {
  std::cerr << "Usage: " << program_name << "-h <ip> -p <port> [-d]"
            << std::endl;
}

int main(int argc, char *argv[]) {
  std::string serverIp = "127.0.0.1";
  int serverPort = 8080;
  bool debugMode = false;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" && i + 1 < argc) {
      serverIp = argv[++i];
    } else if (arg == "-p" && i + 1 < argc) {
      serverPort = std::stoi(argv[++i]);
    } else if (arg == "-d") {
      debugMode = true;
    } else {
      usage(argv[0]);
      return 1;
    }
  }

  Jetpack::Client::NetworkClient client(serverPort, serverIp, debugMode);
  client.connectToServer();
}
