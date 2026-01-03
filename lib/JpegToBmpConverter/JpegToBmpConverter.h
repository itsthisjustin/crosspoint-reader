#pragma once

class FsFile;
class Print;
class ZipFile;

class JpegToBmpConverter {
  static void writeBmpHeader(Print& bmpOut, int width, int height);
  // [COMMENTED OUT] static uint8_t grayscaleTo2Bit(uint8_t grayscale, int x, int y);
  static unsigned char jpegReadCallback(unsigned char* pBuf, unsigned char buf_size,
                                        unsigned char* pBytes_actually_read, void* pCallback_data);

 public:
  enum class ScaleMode {
    FIT,  // Scale to fit within target dimensions (may have letterbox bars)
    FILL  // Scale to fill target dimensions completely (crops overflow)
  };

  static bool jpegFileToBmpStream(FsFile& jpegFile, Print& bmpOut, ScaleMode scaleMode = ScaleMode::FIT);
};
