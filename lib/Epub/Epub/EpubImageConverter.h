#pragma once
#include <cstdint>
#include <string>

class Epub;

// Utility for extracting and converting EPUB images to cached BMPs
class EpubImageConverter {
 public:
  struct ImageInfo {
    std::string bmpPath;  // Path to cached BMP on SD card
    uint16_t width = 0;   // Image width in pixels
    uint16_t height = 0;  // Image height in pixels
    bool success = false;
  };

  // Extract an image from EPUB and convert to BMP
  // chapterPath: the path within the EPUB of the current chapter (e.g., "OEBPS/text/chapter1.xhtml")
  // imgSrc: the src attribute from the img tag (e.g., "../images/fig.jpg")
  // maxWidth: maximum width for the converted image
  static ImageInfo extractAndConvert(Epub& epub, const std::string& chapterPath, const std::string& imgSrc,
                                     uint16_t maxWidth);

 private:
  // Resolve a relative image path against the chapter path
  static std::string resolveImagePath(const std::string& chapterPath, const std::string& imgSrc);

  // Generate a unique cache filename for the image
  static std::string generateCacheFilename(const std::string& imagePath, uint16_t maxWidth);
};
