#pragma once
#include <SdFat.h>

#include <memory>
#include <string>

#include "Block.h"

class GfxRenderer;

// Represents an image on a page
class ImageBlock final : public Block {
 private:
  std::string bmpCachePath;  // Path to cached BMP file on SD card
  uint16_t width;            // Image width in pixels
  uint16_t height;           // Image height in pixels

 public:
  explicit ImageBlock(std::string cachePath, uint16_t w, uint16_t h)
      : bmpCachePath(std::move(cachePath)), width(w), height(h) {}
  ~ImageBlock() override = default;

  bool isEmpty() override { return bmpCachePath.empty(); }
  void layout(GfxRenderer& renderer) override {}
  BlockType getType() override { return IMAGE_BLOCK; }

  uint16_t getWidth() const { return width; }
  uint16_t getHeight() const { return height; }
  const std::string& getBmpPath() const { return bmpCachePath; }

  void render(const GfxRenderer& renderer, int x, int y, int maxWidth, int maxHeight) const;
  bool serialize(FsFile& file) const;
  static std::unique_ptr<ImageBlock> deserialize(FsFile& file);
};
