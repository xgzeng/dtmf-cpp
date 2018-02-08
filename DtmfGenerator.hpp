/** Author:       Plyashkevich Viatcheslav <plyashkevich@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * All rights reserved.
 */
#ifndef _DTMF_GENERATOR_
#define _DTMF_GENERATOR_

#include <stdint.h>

// Class DtmfGenerator is used for generating of DTMF
// frequences, corresponding push buttons.

class DtmfGenerator {
  // Number of buffers a single tone should occupy.
  // Initialized in the constructor.
  const int32_t countDurationPushButton;
  // Number of buffers a single silence should occupy.
  // Initialized in the constructor.
  const int32_t countDurationPause;
  // Number of buffers we have to write to complete the current tone.
  int32_t tempCountDurationPushButton;
  // Number of buffers we have to write to complete the current silence.
  int32_t tempCountDurationPause;
  // Set to 0 while there is still something left to output, i.e. not all
  // of the tones in pushDialButtons have been completely output.  This
  // means: "please wait until I'm done before sending me more input."
  // Set to 1 when we've output everything we've been asked to.  This
  // means: "please give me more input".
  int32_t readyFlag;
  // A fixed-size array of dial tones to generate.
  char pushDialButtons[20];
  // The number of tones we still have to generate (gets decremented
  // each time a tone is generated).
  uint32_t countLengthDialButtonsArray;
  // The index of the current tone we're generating (gets incremented
  // each time a tone is generated).
  uint32_t count;
  // The size of the output buffer we will be writing to.
  int32_t sizeOfFrame;

  short tempCoeff1, tempCoeff2;
  int32_t y1_1, y1_2, y2_1, y2_2;

public:
  // FrameSize - Size of frame, DurationPush - duration pushed button in ms
  // DurationPause - duration pause between pushed buttons in ms
  DtmfGenerator(int32_t FrameSize, int32_t DurationPush = 70,
                int32_t DurationPause = 50);
  ~DtmfGenerator();

  // That function will be run on each outcoming frame
  //
  // This function performs the actual generation of the signal.
  //
  // The size of out (the buffer to which the generated signal will
  // be output to) is SizeOfFrame (specified in constructor.  Does
  // nothing if ready_flag is non-zero.
  void dtmfGenerating(int16_t out[]);

  // If transmitNewDialButtonsArray return 1 then the dialButtonsArray will be
  // transmitted if 0, transmit is not possible and is needed to wait (nothing
  // will be transmitted) Warning! lengthDialButtonsArray must to be < 21 and !=
  // 0, if lengthDialButtonsArray will be > 20 will be transmitted only first 20
  // dial buttons if lengthDialButtonsArray == 0 will be returned 1 and nothing
  // will be transmitted
  int32_t transmitNewDialButtonsArray(char dialButtonsArray[],
                                      uint32_t lengthDialButtonsArray);

  // Reset generation
  void dtmfGeneratorReset() {
    countLengthDialButtonsArray = 0;
    count = 0;
    readyFlag = 1;
  }

  // If getReadyFlag return 1 then a new button's array may be transmitted
  // if 0 transmit is not possible and is needed to wait
  int32_t getReadyFlag() const { return readyFlag ? 1 : 0; }
};

/*			Example:

DtmfGenerator dtmfGen( 256, // frame size
                                                60,  // duration in ms of a
pressure of a button 50   // duration pause between pressures of buttons
                                                );

// |dtmf generation 60 ms|  pause 50 ms	 |dtmf generation 60 ms|  pause
50 ms	 | and so on...

int16_t y[256]; // outcoming frame

char pushButtons[20] = {'1', '2', '#', '*', '0', 'D'....}; // array of push
buttons,
                                                // will be generate dtmf
frequencies corresponding of push buttons
                                                // 1, 2, #, *, 0, D and so on.

volatile int break_current_action = 0;
        while(1)
        {
                dtmfGen.transmitNewDialButtonsArray(pushButtons, 20); // new
generation while(!dtmfGen.getReadyFlag())
                {
                        dtmfGen.dtmfGenerating(y); // in the y will be writed
256 (this size define in constructor)
                        // int16_t dtmf samples, it samples will be replaced
with new 256 samples in each iteration,
                        // after that this array may be transfered to
peripherals or to auxialiry
                        // processing
                        if(break_current_action) // some extern event occur
(interrupt for example) dtmfGen.dtmfGeneratorReset(); // stop current generation
                }
        }

*/

#endif
