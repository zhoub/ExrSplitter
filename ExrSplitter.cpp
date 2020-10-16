/*
 * Copyright 2020 Bo Zhou (bo.schwarzstein@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdlib>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include <OpenImageIO/imageio.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return EXIT_FAILURE;
  }

  // Open file.
  const char *inputFilePath = argv[1];
  auto inputImage = OIIO::ImageInput::open(inputFilePath);
  inputImage->open(inputFilePath);

  // Read image plane with channels.
  const auto &inputImageSpec = inputImage->spec();

  std::map<std::string, int> imageChannelIndices;
  std::map<std::string, std::string> imageChannelComponentNames;
  std::multimap<std::string, std::string> imagePlaneChannelNames;
  std::set<std::string> imagePlaneNames;
  for (int i = 0; i < inputImageSpec.nchannels; i++) {
    const auto &channelName = inputImageSpec.channelnames[i];

    std::string imagePlaneName;
    std::string componentName = channelName;
    const auto dotPos = channelName.find('.');
    if (dotPos != std::string::npos) {
      imagePlaneName = channelName.substr(0, dotPos);
      componentName = channelName.substr(dotPos + 1);
    }

    if (componentName != "R" && componentName != "G" && componentName != "B" &&
        componentName != "A") {
      continue;
    }

    imageChannelIndices.insert(
        std::make_pair(channelName, inputImageSpec.channelindex(channelName)));
    imageChannelComponentNames.insert(
        std::make_pair(channelName, componentName));
    imagePlaneChannelNames.insert(std::make_pair(imagePlaneName, channelName));
    imagePlaneNames.insert(imagePlaneName);
  }

  // Read scanlines from input and save then to output files.
  std::map<std::string, std::shared_ptr<OIIO::ImageOutput>> outputImages;

  std::string outputFilePathNoExt(argv[1]);
  const auto outputFilePathNoExtPos = outputFilePathNoExt.rfind(".");
  outputFilePathNoExt = outputFilePathNoExt.substr(0, outputFilePathNoExtPos);

  for (const auto &imagePlaneName : imagePlaneNames) {
    const auto &outputFilePath =
        outputFilePathNoExt + "_" + (imagePlaneName.empty() ? "RGBA" : imagePlaneName) + ".exr";
    std::cout << outputFilePath << std::endl;

    auto outputImage = OIIO::ImageOutput::create(outputFilePath);
    if (!outputImage) {
      std::cerr << "failed to create image " << outputFilePath << std::endl;
    }

    int channelCount = imagePlaneChannelNames.count(imagePlaneName);
    OIIO::ImageSpec outputImageSpec(inputImageSpec.width, inputImageSpec.height,
                                    channelCount, OIIO::TypeDesc::FLOAT);
    for (int i = 0; i < channelCount; i++) {
      outputImageSpec.channelformats.push_back(OIIO::TypeDesc::FLOAT);
    }

    if (!outputImage->open(outputFilePath, outputImageSpec)) {
      std::cerr << "failed to open image " << outputFilePath << std::endl;
    }
    std::cout << imagePlaneName << " " << channelCount << std::endl;

    outputImages.insert(std::make_pair(imagePlaneName, std::move(outputImage)));
  }

  // Create several .exr file for different image plane.
  for (int y = 0; y < inputImageSpec.height; y++) {
    // Read scanline.
    std::vector<float> data(inputImageSpec.nchannels * inputImageSpec.width);
    if (!inputImage->read_scanline(y, 0, data.data())) {
      std::cout << "failed to read scanline " << y << std::endl;
      return EXIT_FAILURE;
    }

    // Fill portion of scanline for image.
    std::map<std::string, std::vector<float>> scanlines;
    for (const auto &imagePlaneName : imagePlaneNames) {
      int channelCount = imagePlaneChannelNames.count(imagePlaneName);

      scanlines[imagePlaneName] = std::vector<float>();
      auto &scanline = scanlines[imagePlaneName];
      scanline.resize(channelCount * inputImageSpec.width);
    }

    for (const auto &pair : imagePlaneChannelNames) {
      const auto &imagePlaneName = pair.first;
      const auto &channelName = pair.second;

      int channelIndex = inputImageSpec.channelindex(channelName);

      int componentIndex = -1;
      auto componentName = imageChannelComponentNames[channelName];
      if (componentName == "R") {
        componentIndex = 0;
      } else if (componentName == "G") {
        componentIndex = 1;
      } else if (componentName == "B") {
        componentIndex = 2;
      } else if (componentName == "A") {
        componentIndex = 3;
      } else {
        std::cout << "invalid component" << std::endl;
        return EXIT_FAILURE;
      }

      auto &scanline = scanlines[imagePlaneName];
      int imageChannelCount = imagePlaneChannelNames.count(imagePlaneName);
      for (int x = 0; x < inputImageSpec.width; x++) {
        float v = data[channelIndex + x * inputImageSpec.nchannels];
        scanline[componentIndex + x * imageChannelCount] = v;
      }
    }

    for (const auto &imagePlaneName : imagePlaneNames) {
      int channelCount = imagePlaneChannelNames.count(imagePlaneName);

      auto &outputImage = outputImages[imagePlaneName];
      auto &scanline = scanlines[imagePlaneName];
      outputImage->write_scanline(y, 0, OIIO::TypeDesc::FLOAT, scanline.data());
    }
  }

  // Close file.
  inputImage->close();

  for (auto pair : outputImages) {
    pair.second->close();
  }

  return EXIT_SUCCESS;
}
