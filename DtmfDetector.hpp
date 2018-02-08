/** Author:       Plyashkevich Viatcheslav <plyashkevich@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * All rights reserved.
 */


#ifndef DTMF_DETECTOR
#define DTMF_DETECTOR

#include "types_cpp.hpp"


typedef Types<sizeof(long int), sizeof(int), sizeof(short int), sizeof(char)>::Int32     INT32;
typedef Types<sizeof(long int), sizeof(int), sizeof(short int), sizeof(char)>::Uint32    UINT32;
typedef Types<sizeof(long int), sizeof(int), sizeof(short int), sizeof(char)>::Int16     INT16;
typedef Types<sizeof(long int), sizeof(int), sizeof(short int), sizeof(char)>::Uint16    UINT16;


// DTMF detector object

// N.B. Not sure why this interface is necessary, as the only class to 
// implement it is the DtmfDetector.
class DtmfDetectorInterface
{
public:
    // The maximum number of detected tones
    static const UINT32 NUMBER_OF_BUTTONS = 65;
    // A fixed-size array to store the detected tones.  The array is
    // NUL-terminated.
    char dialButtons[NUMBER_OF_BUTTONS];
    // A constant pointer to expose dialButtons for read-only access
    char *const pDialButtons;
    // The number of tones detected (stored in dialButtons)
    mutable INT16 indexForDialButtons;
public:
    INT32 getIndexDialButtons() // The number of detected push buttons, max number = 64
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
    const
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
    INT16 *pArraySamples;
    // The size of the entire buffer used for processing samples.  
    // Specified at construction time.
    const INT32 frameSize; //Size of a frame is measured in INT16(word)

    // This gets used for a variety of purposes.  Most notably, it indicates
    // the start of the circular buffer at the start of ::dtmfDetecting.
    INT32 frameCount;
    // The tone detected by the previous call to DTMF_detection.
    char prevDialButton;
    // This flag is used to aggregate adjacent tones and spaces, i.e.
    //
    // 111111   222222 -> 12 (where a space represents silence)
    //
    // 1 means that during the previous iteration, we entered a tone 
    // (gone from silence to non-silence).
    // 0 means otherwise.
    //
    // While this is a member variable, it can by all means be a local
    // variable in dtmfDetecting.
    //
    // N.B. seems to not work.  In practice, you get this:
    //
    // 111111   222222 -> 1111122222
    char permissionFlag;

public:

    // frameSize_ - input frame size
    DtmfDetector(INT32 frameSize_);
    ~DtmfDetector();

    void dtmfDetecting(const INT16 inputFrame[]); // The DTMF detection.
    // The size of a inputFrame must be equal of a frameSize_, who
    // was set in constructor.
};

#endif
