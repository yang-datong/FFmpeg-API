#include <iostream>

extern "C" {
#include "libavformat/avformat.h"
}

#define INPUT_FILE "input.mov"
using std::string;

int getInfo() {

  int status = -1;

  AVFormatContext *ps = nullptr;
  string url = INPUT_FILE;

  // 1. Open the input file
  status = avformat_open_input(&ps, url.c_str(), nullptr, nullptr);
  if (status != 0) {
    std::cerr << "File " + url + " parse failed" << std::endl;
    return -1;
  }

  // 2. Retrieve stream information
  status = avformat_find_stream_info(ps, nullptr);
  if (status != 0) {
    std::cerr << "Stream find info failed" << std::endl;
    return -1;
  }

  int index = 0;
  int is_output = 0;

  // 3. Print format and stream information
  av_dump_format(ps, index, url.c_str(), is_output);

  // 4. Close the input file IO
  avformat_close_input(&ps);

  return 0;
}
