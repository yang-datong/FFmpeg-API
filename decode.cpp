#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define INPUT_FILE "input.mov"
#define OUTPUT_VIDEO_FILE "output.yuv"
#define OUTPUT_AUDIO_FILE "output.pcm"
#define OUTPUT_SUBTITLE_FILE "output.srt"

using std::string;

int decode() {

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

  AVCodecContext *video_codec_ctx = avcodec_alloc_context3(video_codec);

  if (avcodec_parameters_to_context(
          video_codec_ctx, ps->streams[VIDEO_STREAM_INDEX]->codecpar) < 0) {
    std::cerr << "Could not copy video codec parameters to context"
              << std::endl;
    return -1;
  }

  if (avcodec_open2(video_codec_ctx, video_codec, nullptr) < 0) {
    std::cerr << "Could not open video codec" << std::endl;
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

  AVCodecContext *audio_codec_ctx = avcodec_alloc_context3(audio_codec);
  if (avcodec_parameters_to_context(
          audio_codec_ctx, ps->streams[AUDIO_STREAM_INDEX]->codecpar) < 0) {
    std::cerr << "Could not copy audio codec parameters to context"
              << std::endl;
    return -1;
  }
  if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0) {
    std::cerr << "Could not open audio codec" << std::endl;
    return -1;
  }

  AVPacket pkt;
  while (av_read_frame(ps, &pkt) >= 0) {
    if (pkt.stream_index == VIDEO_STREAM_INDEX) {
      // Fetch video encode data
      // fwrite(pkt.data, 1, pkt.size, outputVideo);

      // Fetch video raw data
      AVFrame *video_frame = av_frame_alloc();
      int ret = avcodec_send_packet(video_codec_ctx, &pkt);
      if (ret < 0) {
        std::cerr << "Error sending video packet to decoder: " << ret
                  << std::endl;
        break;
      }
      while (ret >= 0) {
        ret = avcodec_receive_frame(video_codec_ctx, video_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        } else if (ret < 0) {
          std::cerr << "Error receiving video frame from decoder: " << ret
                    << std::endl;
          break;
        }
        std::cout << "Video frame: " << video_frame->width << "x"
                  << video_frame->height << ", " << video_frame->pts
                  << std::endl;
        // 将帧数据写入本地文件
        fwrite(video_frame->data[0], 1,
               video_frame->linesize[0] * video_codec_ctx->height, outputVideo);
        fwrite(video_frame->data[1], 1,
               video_frame->linesize[1] * video_codec_ctx->height / 2,
               outputVideo);
        fwrite(video_frame->data[2], 1,
               video_frame->linesize[2] * video_codec_ctx->height / 2,
               outputVideo);

        av_frame_unref(video_frame);
      }
      av_frame_free(&video_frame);

    } else if (pkt.stream_index == AUDIO_STREAM_INDEX) {
      // Fetch audio encode data
      // fwrite(pkt.data, 1, pkt.size, outputAudio);

      // Fetch audio raw data
      AVFrame *audio_frame = av_frame_alloc();
      int ret = avcodec_send_packet(audio_codec_ctx, &pkt);
      if (ret < 0) {
        std::cerr << "Error sending audio packet to decoder: " << ret
                  << std::endl;
        break;
      }
      while (ret >= 0) {
        ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        } else if (ret < 0) {
          std::cerr << "Error receiving audio frame from decoder: " << ret
                    << std::endl;
          break;
        }
        std::cout << "Audio frame: " << audio_frame->nb_samples << " samples, "
                  << audio_frame->pts << std::endl;

        // 计算音频帧大小
        int frame_size = av_get_bytes_per_sample(audio_codec_ctx->sample_fmt) *
                         audio_codec_ctx->channels * audio_frame->nb_samples;

        fwrite(audio_frame->data[0], 1, frame_size, outputAudio);

        av_frame_unref(audio_frame);
      }
      av_frame_free(&audio_frame);

    } else if (pkt.stream_index == SUBTITLE_STREAM_INDEX) {
      fwrite(pkt.data, 1, pkt.size, outputSubtitle);
      std::cout << "Subtitle pts " << pkt.pts << std::endl;
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
