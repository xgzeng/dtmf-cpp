/** Author:       Plyashkevich Viatcheslav <plyashkevich@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * All rights reserved.
 */

#include "DtmfDetector.hpp"
#include <cassert>
#include <algorithm>

#if DEBUG
#include <cstdio>
#endif

// This is the same function as in DtmfGenerator.cpp
static inline int32_t MPY48SR(int16_t o16, int32_t o32)
{
    uint32_t   Temp0;
    int32_t    Temp1;
    Temp0 = (((uint16_t)o32 * o16) + 0x4000) >> 15;
    Temp1 = (int16_t)(o32 >> 16) * o16;
    return (Temp1 << 1) + Temp0;
}

// The Goertzel algorithm.
// For a good description and walkthrough, see:
// https://sites.google.com/site/hobbydebraj/home/goertzel-algorithm-dtmf-detection
//
// Koeff0           Coefficient for the first frequency.
// Koeff1           Coefficient for the second frequency.
// arraySamples     Input samples to process.  Must be COUNT elements long.
// Magnitude0       Detected magnitude of the first frequency.
// Magnitude1       Detected magnitude of the second frequency.
// COUNT            The number of elements in arraySamples.  Always equal to
//                  DTMF_DETECTION_BATCH_SIZE in practice.
static void goertzel_filter(int16_t Koeff0, int16_t Koeff1, const int16_t arraySamples[], int32_t *Magnitude0, int32_t *Magnitude1, uint32_t COUNT)
{
    int32_t Temp0, Temp1;
    uint16_t ii;
    // Vk1_0    prev (first frequency)
    // Vk2_0    prev_prev (first frequency)
    //
    // Vk1_1    prev (second frequency)
    // Vk2_0    prev_prev (second frequency)
    int32_t Vk1_0 = 0, Vk2_0 = 0, Vk1_1 = 0, Vk2_1 = 0;

    // Iterate over all the input samples
    // For each sample, process the two frequencies we're interested in:
    // output = Input + 2*coeff*prev - prev_prev
    // N.B. bit-shifting to the left achieves the multiplication by 2.
    for(ii = 0; ii < COUNT; ++ii)
    {
        Temp0 = MPY48SR(Koeff0, Vk1_0 << 1) - Vk2_0 + arraySamples[ii],
        Temp1 = MPY48SR(Koeff1, Vk1_1 << 1) - Vk2_1 + arraySamples[ii];
        Vk2_0 = Vk1_0,
        Vk2_1 = Vk1_1;
        Vk1_0 = Temp0,
        Vk1_1 = Temp1;
    }

    // Magnitude: prev_prev**prev_prev + prev*prev - coeff*prev*prev_prev

    // TODO: what does shifting by 10 bits to the right achieve?  Probably to
    // make room for the magnitude calculations.
    Vk1_0 >>= 10,
              Vk1_1 >>= 10,
                        Vk2_0 >>= 10,
                                  Vk2_1 >>= 10;
    Temp0 = MPY48SR(Koeff0, Vk1_0 << 1),
    Temp1 = MPY48SR(Koeff1, Vk1_1 << 1);
    Temp0 = (int16_t)Temp0 * (int16_t)Vk2_0,
    Temp1 = (int16_t)Temp1 * (int16_t)Vk2_1;
    Temp0 = (int16_t)Vk1_0 * (int16_t)Vk1_0 + (int16_t)Vk2_0 * (int16_t)Vk2_0 - Temp0;
    Temp1 = (int16_t)Vk1_1 * (int16_t)Vk1_1 + (int16_t)Vk2_1 * (int16_t)Vk2_1 - Temp1;
    *Magnitude0 = Temp0,
     *Magnitude1 = Temp1;
    return;
}


// This is a GSM function, for concrete processors she may be replaced
// for same processor's optimized function (norm_l)
// This is a GSM function, for concrete processors she may be replaced
// for same processor's optimized function (norm_l)
//
// This function is used for normalization. TODO: how exactly does it work?
// Result is highest non-zero bit position
static inline int16_t norm_l(int32_t L_var1)
{
    int16_t var_out;
    
    if (L_var1 == 0)
    {
        var_out = 0;
    }
    else
    {
        if (L_var1 == (int32_t)0xffffffff)
        {
            var_out = 31;
        }
        else
        {
            if (L_var1 < 0)
            {
                L_var1 = ~L_var1;
            }

            for(var_out = 0; L_var1 < (int32_t)0x40000000; var_out++)
            {
                L_var1 <<= 1;
            }
        }
    }

    return(var_out);
}

// These coefficients include the 8 DTMF frequencies plus 10 harmonics.
static const unsigned COEFF_NUMBER=18;

// These frequencies are slightly different to what is in the generator.
// More importantly, they are also different to what is described at:
// http://en.wikipedia.org/wiki/Dual-tone_multi-frequency_signaling
//
// Some frequencies seem to match musical notes:
// http://www.geocities.jp/pyua51113/artist_data/ki_setsumei.html 
// http://en.wikipedia.org/wiki/Piano_key_frequencies
//
// It seems this is done to simplify harmonic detection.  
//
// A fixed-size array to hold the coefficients
const int16_t CONSTANTS[COEFF_NUMBER] = {
    27860,  // 0: 706Hz, harmonics include: 78Hz, 235Hz, 3592Hz 
    26745,  // 1: 784Hz, apparently a high G, harmonics: 78Hz
    25529,  // 2: 863Hz, harmonics: 78Hz 
    24216,  // 3: 941Hz, harmonics: 78Hz, 235Hz, 314Hz
    19747,  // 4: 1176Hz, harmonics: 78Hz, 235Hz, 392Hz, 3529Hz
    16384,  // 5: 1333Hz, harmonics: 78Hz 
    12773,  // 6: 1490Hz, harmonics: 78Hz, 2980Hz
    8967,   // 7: 1547Hz, harmonics: 314Hz, 392Hz
    // The next coefficients correspond to frequencies of harmonics of the 
    // near-DTMF frequencies above, as well as of other frequencies listed
    // below.
    21319,  // 1098Hz
    29769,  // 549Hz
    // 549Hz is:
    // - half of 1098Hz (see above)
    // - 1/3 of 1633Hz, a real DTMF frequency (see DtmfGenerator)
    32706,  // 78Hz, a very low D# on a piano
    // 78Hz is a very convenient frequency, since its (approximately): 
    // - 1/3 of 235Hz (not a DTMF frequency, but we do detect it, see below)
    // - 1/4 of 314Hz (not a DTMF frequency, but we do detect it, see below)
    // - 1/5 of 392Hz (not a DTMF frequency, but we do detect it, see below)
    // - 1/7 of 549Hz
    // - 1/9 of 706Hz
    // - 1/10 of 784Hz
    // - 1/11 of 863Hz
    // - 1/12 of 941Hz
    // - 1/14 of 1098Hz (not a DTMF frequency, but we do detect it, see above)
    // - 1/15 of 1176Hz
    // - 1/17 of 1333Hz
    // - 1/19 of 1490Hz
    32210,  // 235Hz
    // 235Hz is:
    // - 1/3 of 706Hz
    // - 1/4 of 941Hz
    // - 1/5 of 1176Hz
    // - 1/15 of 3529Hz (not a DTMF frequency, but we do detect it, see below)
    31778,  // 314Hz
    // 314Hz is:
    // - 1/3 of 941Hz
    // - 1/5 of 1547Hz
    // - 1/8 of 2510Hz (not a DTMF frequency, but we do detect it, see below)
    31226,  // 392Hz, apparently a middle-2 G
    // 392Hz is:
    // - 1/2 of 794Hz
    // - 1/3 of 1176Hz
    // - 1/4 of 1547Hz
    // - 1/9 of 3529Hz
    -1009,  // 2039Hz TODO: why is this frequency useful?
    -12772, // 2510Hz, which is 8*314Hz
    -22811, // 2980Hz, which is 2*1490Hz
    -30555  // 3529Hz, 3*1176Hz, 5*706Hz
};

const int32_t powerThreshold = 328;
const int32_t dialTonesToOhersTones = 16;
const int32_t dialTonesToOhersDialTones = 6;

static char DTMF_detection(const int16_t short_array_samples[]);

//--------------------------------------------------------------------
DtmfDetectorBase::DtmfDetectorBase()
{
    buf_sample_count_ = 0;
    prev_dial_ = ' ';
}

void DtmfDetectorBase::dtmfDetecting(const int16_t *samples, int sample_count)
{
    if (buf_sample_count_ != 0) {
        // Copy the input array into the back of buf_samples_.
        int count_to_copy = std::min(sample_count, DTMF_DETECTION_BATCH_SIZE - buf_sample_count_);
        std::copy(samples, samples + count_to_copy, buf_samples_ + buf_sample_count_);
        buf_sample_count_ += count_to_copy;
        samples += count_to_copy;
        sample_count -= count_to_copy;
        if (sample_count < DTMF_DETECTION_BATCH_SIZE) {
          return;
        }
    }

    // process batch samples in buffer
    if (buf_sample_count_ == DTMF_DETECTION_BATCH_SIZE) {
        char dial_char = DTMF_detection(buf_samples_);
        OnDetectedTone(dial_char);
        buf_sample_count_ = 0;
    }

    // process samples in input data directory
    while (sample_count >= DTMF_DETECTION_BATCH_SIZE)
    {
        // Determine the tone present in the current batch
        char dial_char = DTMF_detection(samples);
        OnDetectedTone(dial_char);

        samples += DTMF_DETECTION_BATCH_SIZE;
        sample_count -= DTMF_DETECTION_BATCH_SIZE;
    }

    // We have sample_count samples left to process, but it's not enough for an entire batch. 
    // Store the samples to the buffer and deal with them
    // next time this function is called.
    assert(buf_sample_count_ == 0 && sample_count < DTMF_DETECTION_BATCH_SIZE);
    std::copy(samples, samples + sample_count, buf_samples_);
    buf_sample_count_ = sample_count;
}

void DtmfDetectorBase::OnDetectedTone(char dial_char) {
  // Determine if we should register it as a new tone, or
  // ignore it as a continuation of a previous tone.  
  if (dial_char != prev_dial_ && dial_char != ' ') {
    OnNewTone(dial_char);
  }

  // Store the current tone.
  prev_dial_ = dial_char;
}

//-----------------------------------------------------------------
// Detect a tone in a single batch of samples (DTMF_DETECTION_BATCH_SIZE elements).
char DTMF_detection(const int16_t short_array_samples[])
{
    // The magnitude of each coefficient in the current frame.  Populated
    // by goertzel_filter
    int32_t T[COEFF_NUMBER];
    
    // An array of size DTMF_DETECTION_BATCH_SIZE.  Used as input to the Goertzel function.
    int16_t internalArray[DTMF_DETECTION_BATCH_SIZE];
    
    char return_value=' ';
    unsigned ii;

    // Dial         TODO: what is this?
    // return_value The tone detected in this batch (can be silence).
    // ii           Iteration variable

    {
      // Quick check for silence by calculate average magnitude
      int32_t Sum = 0;
      for(ii = 0; ii < DTMF_DETECTION_BATCH_SIZE; ii ++)
      {
          Sum += abs(short_array_samples[ii]);
          //if(short_array_samples[ii] >= 0)
          //    Sum += short_array_samples[ii];
          //else
          //    Sum -= short_array_samples[ii];
      }
      Sum /= DTMF_DETECTION_BATCH_SIZE;
      if(Sum < powerThreshold)
          return ' ';
    }

    // Normalization
    {
      // Iterate over each sample.
      // First, adjusting Dial to an appropriate value for the batch.
      int32_t Dial = 32;
      for(ii = 0; ii < DTMF_DETECTION_BATCH_SIZE; ii++)
      {
          int32_t sample32 = static_cast<int32_t>(short_array_samples[ii]);
          if(sample32 != 0)
          {
              if(Dial > norm_l(sample32))
              {
                  Dial = norm_l(sample32);
              }
          }
      }

      Dial -= 16;

      // Next, utilize Dial for scaling and populate internalArray.
      for(ii = 0; ii < DTMF_DETECTION_BATCH_SIZE; ii++)
      {
          int32_t sample32 = short_array_samples[ii];
          internalArray[ii] = static_cast<int16_t>(sample32 << Dial);
      }
    }

    //Frequency detection
    goertzel_filter(CONSTANTS[0], CONSTANTS[1], internalArray, &T[0], &T[1], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[2], CONSTANTS[3], internalArray, &T[2], &T[3], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[4], CONSTANTS[5], internalArray, &T[4], &T[5], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[6], CONSTANTS[7], internalArray, &T[6], &T[7], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[8], CONSTANTS[9], internalArray, &T[8], &T[9], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[10], CONSTANTS[11], internalArray, &T[10], &T[11], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[12], CONSTANTS[13], internalArray, &T[12], &T[13], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[14], CONSTANTS[15], internalArray, &T[14], &T[15], DTMF_DETECTION_BATCH_SIZE);
    goertzel_filter(CONSTANTS[16], CONSTANTS[17], internalArray, &T[16], &T[17], DTMF_DETECTION_BATCH_SIZE);

#if DEBUG
    for (ii = 0; ii < COEFF_NUMBER; ++ii)
        printf("%d ", T[ii]);
    printf("\n");
#endif

    int32_t Row = 0;
    int32_t Temp = 0;
    // Row      Index of the maximum row frequency in T
    // Temp     The frequency at the maximum row/column (gets reused 
    //          below).
    //Find max row(low frequences) tones
    for(int ii = 0; ii < 4; ii++)
    {
        if(Temp < T[ii])
        {
            Row = ii;
            Temp = T[ii];
        }
    }

    // Column   Index of the maximum column frequency in T
    int32_t Column = 4;
    Temp = 0;
    //Find max column(high frequences) tones
    for(int ii = 4; ii < 8; ii++)
    {
        if(Temp < T[ii])
        {
            Column = ii;
            Temp = T[ii];
        }
    }

    int32_t Sum = 0;
    //Find average value dial tones without max row and max column
    for(ii = 0; ii < 10; ii++)
    {
        Sum += T[ii];
    }
    Sum -= T[Row];
    Sum -= T[Column];
    // N.B. Divide by 8
    Sum >>= 3;

    // N.B. looks like avoiding a divide by zero.
    if(!Sum)
        Sum = 1;

    //If relations max row and max column to average value
    //are less then threshold then return
    // This means the tones are too quiet compared to the other, non-max
    // DTMF frequencies.
    if(T[Row]/Sum < dialTonesToOhersDialTones)
        return ' ';
    if(T[Column]/Sum < dialTonesToOhersDialTones)
        return ' ';

    // Next, check if the volume of the row and column frequencies
    // is similar.  If they are different, then they aren't part of
    // the same tone.
    //
    // In the literature, this is known as "twist".
    //If relations max colum to max row is large then 4 then return
    if(T[Row] < (T[Column] >> 2)) return ' ';
    //If relations max colum to max row is large then 4 then return
    // The reason why the twist calculations aren't symmetric is that the
    // allowed ratios for normal and reverse twist are different.
    if(T[Column] < ((T[Row] >> 1) - (T[Row] >> 3))) return ' ';

    // N.B. looks like avoiding a divide by zero.
    for(ii = 0; ii < COEFF_NUMBER; ii++)
        if(T[ii] == 0)
            T[ii] = 1;

    //If relations max row and max column to all other tones are less then
    //threshold then return
    // Check for the presence of strong harmonics.
    for(ii = 10; ii < COEFF_NUMBER; ii ++)
    {
        if(T[Row]/T[ii] < dialTonesToOhersTones)
            return ' ';
        if(T[Column]/T[ii] < dialTonesToOhersTones)
            return ' ';
    }


    //If relations max row and max column tones to other dial tones are
    //less then threshold then return
    for(ii = 0; ii < 10; ii ++)
    {
        // TODO:
        // The next two nested if's can be collapsed into a single
        // if-statement.  Basically, he's checking that the current
        // tone is NOT the maximum tone.
        //
        // A simpler check would have been (ii != Column && ii != Row)
        //
        if(T[ii] != T[Column])
        {
            if(T[ii] != T[Row])
            {
                if(T[Row]/T[ii] < dialTonesToOhersDialTones)
                    return ' ';
                if(Column != 4)
                {
                    // Column == 4 corresponds to 1176Hz.
                    // TODO: what is so special about this frequency?
                    if(T[Column]/T[ii] < dialTonesToOhersDialTones)
                        return ' ';
                }
                else
                {
                    if(T[Column]/T[ii] < (dialTonesToOhersDialTones/3))
                        return ' ';
                }
            }
        }
    }

    //We are choosed a push button
    // Determine the tone based on the row and column frequencies.
    switch (Row)
    {
    case 0:
        switch (Column)
        {
        case 4:
            return_value='1';
            break;
        case 5:
            return_value='2';
            break;
        case 6:
            return_value='3';
            break;
        case 7:
            return_value='A';
            break;
        };
        break;
    case 1:
        switch (Column)
        {
        case 4:
            return_value='4';
            break;
        case 5:
            return_value='5';
            break;
        case 6:
            return_value='6';
            break;
        case 7:
            return_value='B';
            break;
        };
        break;
    case 2:
        switch (Column)
        {
        case 4:
            return_value='7';
            break;
        case 5:
            return_value='8';
            break;
        case 6:
            return_value='9';
            break;
        case 7:
            return_value='C';
            break;
        };
        break;
    case 3:
        switch (Column)
        {
        case 4:
            return_value='*';
            break;
        case 5:
            return_value='0';
            break;
        case 6:
            return_value='#';
            break;
        case 7:
            return_value='D';
            break;
        }
    }

    return return_value;
}
