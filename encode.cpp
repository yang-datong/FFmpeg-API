#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#define INPUT_FILE "input.yuv"
#define OUTPUT_FILE "output.h264"
#define WIDTH 1440
#define HEIGHT 1920
#define FRAME_RATE 25
#define BIT_RATE 100 * 1000

using std::string;

int encode() {

  FILE *inputFile = fopen(INPUT_FILE, "rb");
  if (!inputFile) {
    std::cerr << "Could not open input file" << std::endl;
    return -1;
  }

  // 1. Find encoder
  AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    std::cerr << "Codec not found" << std::endl;
    return -1;
  }

  // 2. Create encoder context
  AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
  if (!codecCtx) {
    std::cerr << "Could not allocate video codec context" << std::endl;
    return -1;
  }

  // 3. Configure encoder parameters
  codecCtx->bit_rate = BIT_RATE;
  codecCtx->width = WIDTH;
  codecCtx->height = HEIGHT;
  codecCtx->time_base = (AVRational){1, FRAME_RATE};
  codecCtx->framerate = (AVRational){FRAME_RATE, 1};
  codecCtx->gop_size = 10;
  codecCtx->max_b_frames = 1;
  codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

  if (codec->id == AV_CODEC_ID_H264) {
    av_opt_set(codecCtx->priv_data, "preset", "slow", 0);
  }

  // 4. Open target encoder
  if (avcodec_open2(codecCtx, codec, NULL) < 0) {
    std::cerr << "Could not open codec" << std::endl;
    return -1;
  }

  // 5. Create frame and Image Buffer Zone
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Could not allocate video frame" << std::endl;
    return -1;
  }

  frame->width = WIDTH;
  frame->height = HEIGHT;
  frame->format = AV_PIX_FMT_YUV420P;

  // 6. Create output file
  AVFormatContext *fmtCtx = NULL;
  if (avformat_alloc_output_context2(&fmtCtx, NULL, NULL, OUTPUT_FILE) < 0) {
    std::cerr << "Could not create output context" << std::endl;
    return -1;
  }

  // 7. Create output stream
  AVStream *videoStream = avformat_new_stream(fmtCtx, codec);
  // AVStream *videoStream = avformat_new_stream(fmtCtx, NULL);
  if (!videoStream) {
    std::cerr << "Failed to allocate stream" << std::endl;
    return -1;
  }

  // 8. Set output stream parameters
  if (avcodec_parameters_from_context(videoStream->codecpar, codecCtx) < 0) {
    std::cerr << "Failed to copy codec parameters to stream." << std::endl;
    return -1;
  }

  // 9. Open output file
  if (avio_open(&fmtCtx->pb, OUTPUT_FILE, AVIO_FLAG_WRITE) < 0) {
    std::cerr << "Could not open output file" << std::endl;
    return -1;
  }

  std::cout << "\033[32mWriting to file heard ...\033[0m" << std::endl;
  if (avformat_write_header(fmtCtx, NULL) < 0) {
    std::cerr << "Error occurred when opening output file" << std::endl;
    return -1;
  }

  // 11. Encoding each frame
  int y_size = codecCtx->width * codecCtx->height;
  uint8_t *inData = (uint8_t *)malloc(y_size * 3 / 2);

  AVPacket pkt;
  av_init_packet(&pkt);
  int ret;

  int frameCnt = 0;
  while (true) {
    // Read a frame of YUV image data from the input file
    ret = fread(inData, 1, y_size * 3 / 2, inputFile);
    if (ret != y_size * 3 / 2) {
      break;
    }
    av_image_fill_arrays(frame->data, frame->linesize, inData,
                         codecCtx->pix_fmt, codecCtx->width, codecCtx->height,
                         1);

    // Set frame time-stamp
    frame->pts =
        frameCnt * videoStream->time_base.den / videoStream->time_base.num;

    // Encode frame
    ret = avcodec_send_frame(codecCtx, frame);
    if (ret < 0) {
      std::cerr << "Error sending a frame for encoding" << std::endl;
      return -1;
    }

    while (ret >= 0) {
      ret = avcodec_receive_packet(codecCtx, &pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      } else if (ret < 0) {
        std::cerr << "Error during encoding" << std::endl;
        return -1;
      }

      // Write pack into the output file
      pkt.stream_index = videoStream->index;
      av_packet_rescale_ts(&pkt, codecCtx->time_base, videoStream->time_base);
      if (av_interleaved_write_frame(fmtCtx, &pkt) < 0) {
        std::cerr << "Error writing packet to file." << std::endl;
        return -1;
      }

      av_packet_unref(&pkt);
    }

    frameCnt++;

    std::cout << "Encoding total frame : "  << frameCnt << std::endl;
  }


  std::cout << "Encoding the remaining frame ..." << std::endl;
  ret = avcodec_send_frame(codecCtx, NULL);
  if (ret < 0) {
    std::cerr << "Error sending a frame for encoding" << std::endl;
    return 1;
  }

  while (ret >= 0) {
    ret = avcodec_receive_packet(codecCtx, &pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      std::cerr << "Error during encoding" << std::endl;
      return -1;
    }

    // Write pack into the output file
    pkt.stream_index = videoStream->index;
    av_packet_rescale_ts(&pkt, codecCtx->time_base, videoStream->time_base);
    av_interleaved_write_frame(fmtCtx, &pkt);
    av_packet_unref(&pkt);
  }


  std::cout << "\033[32mWriting to file tail ...\033[0m" << std::endl;
  av_write_trailer(fmtCtx);

  // Close resources
  avcodec_free_context(&codecCtx);
  avformat_free_context(fmtCtx);
  av_frame_free(&frame);
  avio_closep(&fmtCtx->pb);
  free(inData);
  fclose(inputFile);

  return 0;
}
