#include <getopt.h>
#include <iostream>

#include "decode.hpp"
#include "encode.hpp"
#include "getinfo.hpp"
#include "separate.hpp"

#define VERSION 1

std::string file = "";

enum Model { GETINFO, SEPARATE, ENCODE, DECODE };

void CheckArgument(int argc, char *argv[], Model &model) {
  const char *short_opts = "hvgsedf:";
  const option long_opts[] = {{"help", no_argument, nullptr, 'h'},
                              {"version", no_argument, nullptr, 'v'},
                              {"getinfo", required_argument, nullptr, 'g'},
                              {"separate", required_argument, nullptr, 's'},
                              {"encode", required_argument, nullptr, 'e'},
                              {"decode", required_argument, nullptr, 'd'},
                              {"file", required_argument, nullptr, 'f'},
                              {nullptr, no_argument, nullptr, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) !=
         -1) {
    switch (opt) {
    case 'h':
      std::cout << "Usage: " << argv[0]
                << " [options] [--] argument-1 argument-2 \
                    \n\n  Options:\
                      \n  -h, --help           Display this message\
                      \n  -v, --version        Display version\
                      \n  -g, --getinfo        Detection file. input.mov \
                      \n  -s, --separate       Separate file. input.mov -> divide.video,divide.audio,divide.srt \
                      \n  -e, --encode         Encode file. input.yuv -> output.h264 \
                      \n  -d, --decode         Decode file. input.mov -> output.yuv \
                      \n  -f, --file FILE      Test input argument\
                      \n";
      exit(0);
    case 'v':
      std::cout << "Version: " << VERSION << "\n";
      exit(0);
    case 'g':
      model = GETINFO;
      break;
    case 's':
      model = SEPARATE;
      break;
    case 'e':
      model = ENCODE;
      break;
    case 'd':
      model = DECODE;
      break;
    case 'f':
      file = optarg;
      break;
    default:
      std::cout << "Unknown option: " << opt << "\n";
      exit(1);
    }
  }
}

int main(int argc, char *argv[]) {
  Model model;
  CheckArgument(argc, argv, model);
  switch (model) {
  case GETINFO:
    getInfo();
    break;
  case SEPARATE:
    separate();
    break;
  case ENCODE:
    encode();
    break;
  case DECODE:
    decode();
    break;
  default:
    std::cout << "Try --help" << std::endl;
  }

  return 0;
}
