#include "ImageBlock.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HardwareSerial.h>
#include <SDCardManager.h>
#include <Serialization.h>

void ImageBlock::render(const GfxRenderer& renderer, const int x, const int y, const int maxWidth,
                        const int maxHeight) const {
  if (bmpCachePath.empty()) {
    Serial.printf("[%lu] [IMG] Render skipped: empty cache path\n", millis());
    return;
  }

  FsFile file;
  if (!SdMan.openFileForRead("IMG", bmpCachePath, file)) {
    Serial.printf("[%lu] [IMG] Failed to open BMP: %s\n", millis(), bmpCachePath.c_str());
    return;
  }

  Bitmap bitmap(file);
  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    Serial.printf("[%lu] [IMG] Failed to parse BMP headers: %s\n", millis(), bmpCachePath.c_str());
    file.close();
    return;
  }

  renderer.drawBitmap(bitmap, x, y, maxWidth, maxHeight);
  file.close();
}

bool ImageBlock::serialize(FsFile& file) const {
  serialization::writeString(file, bmpCachePath);
  serialization::writePod(file, width);
  serialization::writePod(file, height);
  return true;
}

std::unique_ptr<ImageBlock> ImageBlock::deserialize(FsFile& file) {
  std::string cachePath;
  uint16_t w, h;

  serialization::readString(file, cachePath);
  serialization::readPod(file, w);
  serialization::readPod(file, h);

  // Sanity check
  if (cachePath.empty() || w == 0 || h == 0) {
    Serial.printf("[%lu] [IMG] Deserialization failed: invalid data (path=%s, w=%u, h=%u)\n", millis(),
                  cachePath.c_str(), w, h);
    return nullptr;
  }

  return std::unique_ptr<ImageBlock>(new ImageBlock(std::move(cachePath), w, h));
}
