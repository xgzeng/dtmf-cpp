/** Author:       Plyashkevich Viatcheslav <plyashkevich@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * All rights reserved.
 */

#ifndef DTMF_DETECTOR
#define DTMF_DETECTOR

#include <cstdint>
#include <string>

const int DTMF_DETECTION_BATCH_SIZE = 102;

// DTMF detector object
class DtmfDetectorBase 
{
private:
  // This array keeps the entire buffer PLUS a single batch.
  int16_t buf_samples_[DTMF_DETECTION_BATCH_SIZE];

  // This gets used for a variety of purposes.  Most notably, it indicates
  // the start of the circular buffer at the start of ::dtmfDetecting.
  int buf_sample_count_;

  // The tone detected by the previous call to DTMF_detection.
  char prev_dial_;

  void OnDetectedTone(char dial_char);

protected:
  virtual void OnNewTone(char dial_char) = 0;

public:
  DtmfDetectorBase();

  void dtmfDetecting(const int16_t *input_samples, int sample_count);
};

class DtmfDetector : public DtmfDetectorBase
{
public:
  const std::string& GetResult() const {
    return detected_dial;
  }

  void ClearResult() {
    detected_dial.clear();
  }

private:
  std::string detected_dial;

  void OnNewTone(char dial_char) override {
    detected_dial += dial_char;
  }
};


#endif
