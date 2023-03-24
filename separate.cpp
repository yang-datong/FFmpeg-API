#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define INPUT_FILE "input.mov"
#define OUTPUT_VIDEO_FILE "divide.video"
#define OUTPUT_AUDIO_FILE "divide.audio"
#define OUTPUT_SUBTITLE_FILE "divide.srt"

using std::string;

int separate(){

  int status = -1;

  AVFormatContext *ps = nullptr;
  string url = INPUT_FILE;

  FILE *outputVideo = fopen(OUTPUT_VIDEO_FILE, "wb");
  if (!outputVideo) {
    std::cerr << "Failed to open output video file" << std::endl;
    return -1;
  }

  FILE *outputAudio = fopen(OUTPUT_AUDIO_FILE, "wb");
  if (!outputAudio) {
    std::cerr << "Failed to open output audio file" << std::endl;
    return -1;
  }

  FILE *outputSubtitle = fopen(OUTPUT_SUBTITLE_FILE, "wb");
  if (!outputSubtitle) {
    std::cerr << "Failed to open output subtitle file" << std::endl;
    return -1;
  }

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

  // int index = 0;
  // int is_output = 0;
  //// 3. Print format and stream information
  // av_dump_format(ps, index, url.c_str(), is_output);

#define VIDEO_STREAM_INDEX 0
#define AUDIO_STREAM_INDEX 1
#define SUBTITLE_STREAM_INDEX 2

  string video_type = "";
  string audio_type = "";
  // 4. Find  the video decoder and fetch video data
  const AVCodec *video_codec =
      avcodec_find_decoder(ps->streams[VIDEO_STREAM_INDEX]->codecpar->codec_id);
  if (video_codec) {
    // video_type = video_codec->name; //Better
    video_type = avcodec_get_name(video_codec->id); // Debug test
    std::cout << "Video codec: " << video_type << std::endl;
  } else {
    std::cerr << "Find video codec failed !" << std::endl;
    return -1;
  }

  const AVCodec *audio_codec =
      avcodec_find_decoder(ps->streams[AUDIO_STREAM_INDEX]->codecpar->codec_id);
  if (audio_codec) {
    // audio_type = audio_codec->name;  //Better
    audio_type = avcodec_get_name(audio_codec->id); // Debug test
    std::cout << "Audio codec: " << audio_type << std::endl;
  } else {
    std::cerr << "Find audio codec failed !" << std::endl;
    return -1;
  }


  AVPacket pkt;
  while (av_read_frame(ps, &pkt) >= 0) {
    if (pkt.stream_index == VIDEO_STREAM_INDEX) {
      // Fetch video encode data
      fwrite(pkt.data, 1, pkt.size, outputVideo);
    } else if (pkt.stream_index == AUDIO_STREAM_INDEX) {
      // Fetch audio encode data
      fwrite(pkt.data, 1, pkt.size, outputAudio);
    } else if (pkt.stream_index == SUBTITLE_STREAM_INDEX) {
      fwrite(pkt.data, 1, pkt.size, outputSubtitle);
    }
    // Close data packet resource
    av_packet_unref(&pkt);
  }

  std::cout << "Output video file -> " << OUTPUT_VIDEO_FILE << std::endl;
  std::cout << "Output audio file -> " << OUTPUT_AUDIO_FILE << std::endl;
  std::cout << "Output subtitle file -> " << OUTPUT_SUBTITLE_FILE << std::endl;

  fclose(outputVideo);
  fclose(outputAudio);
  fclose(outputSubtitle);

  // 4. Close the input file IO
  avformat_close_input(&ps);

  return 0;
}
