/** Author:       Plyashkevich Viatcheslav <plyashkevich@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * All rights reserved.
 */


#ifndef DTMF_DETECTOR
#define DTMF_DETECTOR

#include <cstdint>

// DTMF detector object

// N.B. Not sure why this interface is necessary, as the only class to 
// implement it is the DtmfDetector.
class DtmfDetectorInterface
{
public:
    // The maximum number of detected tones
    static const uint32_t NUMBER_OF_BUTTONS = 65;
    // A fixed-size array to store the detected tones.  The array is
    // NUL-terminated.
    char dialButtons[NUMBER_OF_BUTTONS];
    // A constant pointer to expose dialButtons for read-only access
    char *const pDialButtons;
    // The number of tones detected (stored in dialButtons)
    int16_t indexForDialButtons;

public:
    int32_t getIndexDialButtons() // The number of detected push buttons, max number = 64
    const
    {
        return indexForDialButtons;
    }
    char *getDialButtonsArray() // Address of array, where store detected push buttons
    const
    {
        return pDialButtons;
    }
    void zerosIndexDialButton() // Zeros of a detected button array
    {
        indexForDialButtons = 0;
    }

    DtmfDetectorInterface():indexForDialButtons(0), pDialButtons(dialButtons)
    {
        dialButtons[0] = 0;
    }
};

class DtmfDetector : public DtmfDetectorInterface
{
protected:
    // This array keeps the entire buffer PLUS a single batch.
    int16_t *pArraySamples;
    // The size of the entire buffer used for processing samples.  
    // Specified at construction time.
    const int32_t frameSize; //Size of a frame is measured in INT16(word)

    // This gets used for a variety of purposes.  Most notably, it indicates
    // the start of the circular buffer at the start of ::dtmfDetecting.
    int32_t frameCount;
    // The tone detected by the previous call to DTMF_detection.
    char prevDialButton;

    virtual void OnDetectedTone(char dial_char);

public:

    // frameSize_ - input frame size
    DtmfDetector(int32_t frameSize_);
    ~DtmfDetector();

    void dtmfDetecting(const int16_t inputFrame[]); // The DTMF detection.
    // The size of a inputFrame must be equal of a frameSize_, who
    // was set in constructor.
};

#endif
