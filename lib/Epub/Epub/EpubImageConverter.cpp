#include "EpubImageConverter.h"

#include <Bitmap.h>
#include <FsHelpers.h>
#include <HardwareSerial.h>
#include <JpegToBmpConverter.h>
#include <SDCardManager.h>

#include "Epub.h"

std::string EpubImageConverter::resolveImagePath(const std::string& chapterPath, const std::string& imgSrc) {
  // If imgSrc is an absolute path (starts with /), use it directly
  if (!imgSrc.empty() && imgSrc[0] == '/') {
    return imgSrc.substr(1);  // Remove leading /
  }

  // Get the directory of the chapter file
  std::string chapterDir;
  const size_t lastSlash = chapterPath.find_last_of('/');
  if (lastSlash != std::string::npos) {
    chapterDir = chapterPath.substr(0, lastSlash + 1);
  }

  // Combine chapter directory with relative image path
  return FsHelpers::normalisePath(chapterDir + imgSrc);
}

std::string EpubImageConverter::generateCacheFilename(const std::string& imagePath, const uint16_t maxWidth) {
  // Generate a hash-based filename to avoid path issues and duplicates
  const size_t pathHash = std::hash<std::string>{}(imagePath);
  return "img_" + std::to_string(pathHash) + "_" + std::to_string(maxWidth) + ".bmp";
}

EpubImageConverter::ImageInfo EpubImageConverter::extractAndConvert(Epub& epub, const std::string& chapterPath,
                                                                    const std::string& imgSrc,
                                                                    const uint16_t maxWidth) {
  ImageInfo result;

  // Only support JPEG images
  const std::string srcLower = imgSrc;
  const bool isJpeg = (imgSrc.length() >= 4 && (imgSrc.substr(imgSrc.length() - 4) == ".jpg" ||
                                                imgSrc.substr(imgSrc.length() - 4) == ".JPG")) ||
                      (imgSrc.length() >= 5 && (imgSrc.substr(imgSrc.length() - 5) == ".jpeg" ||
                                                imgSrc.substr(imgSrc.length() - 5) == ".JPEG"));

  if (!isJpeg) {
    Serial.printf("[%lu] [EIC] Skipping non-JPEG image: %s\n", millis(), imgSrc.c_str());
    return result;
  }

  // Resolve the full image path within the EPUB
  const std::string imagePath = resolveImagePath(chapterPath, imgSrc);
  Serial.printf("[%lu] [EIC] Resolved image path: %s -> %s\n", millis(), imgSrc.c_str(), imagePath.c_str());

  // Create images cache directory
  const std::string imagesCacheDir = epub.getCachePath() + "/images";
  SdMan.mkdir(imagesCacheDir.c_str());

  // Generate cache filename and full path
  const std::string cacheFilename = generateCacheFilename(imagePath, maxWidth);
  result.bmpPath = imagesCacheDir + "/" + cacheFilename;

  // Check if already cached
  if (SdMan.exists(result.bmpPath.c_str())) {
    // Read dimensions from cached BMP
    FsFile bmpFile;
    if (SdMan.openFileForRead("EIC", result.bmpPath, bmpFile)) {
      Bitmap bitmap(bmpFile);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        result.width = bitmap.getWidth();
        result.height = bitmap.getHeight();
        result.success = true;
        Serial.printf("[%lu] [EIC] Using cached image: %s (%dx%d)\n", millis(), cacheFilename.c_str(), result.width,
                      result.height);
      }
      bmpFile.close();
    }
    return result;
  }

  // Extract JPEG from EPUB to temp file
  const std::string tempJpgPath = epub.getCachePath() + "/.tmp_img.jpg";
  {
    FsFile tempJpg;
    if (!SdMan.openFileForWrite("EIC", tempJpgPath, tempJpg)) {
      Serial.printf("[%lu] [EIC] Failed to create temp JPEG file\n", millis());
      return result;
    }

    if (!epub.readItemContentsToStream(imagePath, tempJpg, 1024)) {
      Serial.printf("[%lu] [EIC] Failed to extract image from EPUB: %s\n", millis(), imagePath.c_str());
      tempJpg.close();
      SdMan.remove(tempJpgPath.c_str());
      return result;
    }
    tempJpg.close();
  }

  // Convert JPEG to BMP
  FsFile jpgFile;
  if (!SdMan.openFileForRead("EIC", tempJpgPath, jpgFile)) {
    Serial.printf("[%lu] [EIC] Failed to open temp JPEG for reading\n", millis());
    SdMan.remove(tempJpgPath.c_str());
    return result;
  }

  FsFile bmpFile;
  if (!SdMan.openFileForWrite("EIC", result.bmpPath, bmpFile)) {
    Serial.printf("[%lu] [EIC] Failed to create BMP file\n", millis());
    jpgFile.close();
    SdMan.remove(tempJpgPath.c_str());
    return result;
  }

  // Use FIT mode for in-chapter images (scale to fit page width)
  const bool converted = JpegToBmpConverter::jpegFileToBmpStream(jpgFile, bmpFile, JpegToBmpConverter::ScaleMode::FIT);
  jpgFile.close();
  bmpFile.close();
  SdMan.remove(tempJpgPath.c_str());

  if (!converted) {
    Serial.printf("[%lu] [EIC] Failed to convert JPEG to BMP: %s\n", millis(), imagePath.c_str());
    SdMan.remove(result.bmpPath.c_str());
    return result;
  }

  // Read dimensions from converted BMP
  if (SdMan.openFileForRead("EIC", result.bmpPath, bmpFile)) {
    Bitmap bitmap(bmpFile);
    if (bitmap.parseHeaders() == BmpReaderError::Ok) {
      result.width = bitmap.getWidth();
      result.height = bitmap.getHeight();
      result.success = true;
      Serial.printf("[%lu] [EIC] Converted image: %s (%dx%d)\n", millis(), cacheFilename.c_str(), result.width,
                    result.height);
    }
    bmpFile.close();
  }

  if (!result.success) {
    SdMan.remove(result.bmpPath.c_str());
  }

  return result;
}
