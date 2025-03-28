#include <iostream>

static void usage(char *program_name) {
  std::cerr << "Usage: " << program_name << "-p <port> -m <map> [-d]"
            << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    usage(argv[0]);
    return 84;
  }

  std::cout << "Hello world !" << std::endl;
  return 0;
}
