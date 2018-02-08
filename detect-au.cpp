//
// Utilize the DtmfDetector to detect tones in an AU file.
// The file must be 8KHz, PCM encoded, mono.
//

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdint.h>

#include "DtmfDetector.hpp"

//
// The string ".snd" in big-endian byte ordering.  This identifies the file as
// an AU sound file.
//
#define AU_MAGIC 0x2e736e64

//
// The size of the buffer we use for reading & processing the audio samples.
//

using namespace std;

//
// Swap the endianness of an 4-byte integer.
//
uint32_t
swap32(uint32_t a)
{
    char b1, b2, b3, b4 = 0;
    b1 = a;
    b2 = a >> 8;
    b3 = a >> 16;
    b4 = a >> 24;
    return (b1 << 24) + (b2 << 16) + (b3 << 8) + b4;
}

void swap16_inplace(uint16_t& a) {
  a = ((a & 0x00FF) << 8) | ((a & 0xFF00) >> 8);
}

void swap16_inplace(int16_t& a) {
  a = ((a & 0x00FF) << 8) | ((a & 0xFF00) >> 8);
}

struct au_header
{
    uint32_t magic;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t encoding;
    uint32_t sample_rate;
    uint32_t nchannels;
};

string
au_header_tostr(au_header &h)
{
    stringstream ss;
    ss << h.data_offset << " header bytes, " <<
        h.data_size << " data bytes, encoding type: " << h.encoding << ", "
        << h.sample_rate << "Hz, " << h.nchannels << " channels";
    return ss.str();
}

int
main(int argc, char **argv)
{
    if (argc != 2)
    {
        cerr << "usage: " << argv[0] << " filename.au" << endl;
        return 1;
    }

    ifstream fin(argv[1], ios::binary);
    if (!fin.good())
    {
        cerr << argv[1] << ": unable to open file" << endl;
        return 1;
    }
    au_header header;
    fin.read((char *)&header, sizeof(au_header));

    //
    // The data in the AU file header is stored in big-endian byte ordering.
    // If we're on a little-endian machine, we need to reorder the bytes.
    // While other arrangements (e.g. middle-endian) are also technically
    // possible, this example does not support them, since they are relatively
    // rare.
    //
    bool swap_endian = false;
    if (header.magic == AU_MAGIC)
    {
      swap_endian = false;
    }
    else if (header.magic == swap32(AU_MAGIC))
    {
        swap_endian = true;
        header.data_offset = swap32(header.data_offset);
        header.data_size = swap32(header.data_size);
        header.encoding = swap32(header.encoding);
        header.sample_rate = swap32(header.sample_rate);
        header.nchannels = swap32(header.nchannels);
    }
    else
    {
        cerr << "bad magic number: " << hex << header.magic << endl;
        return 1;
    }

    cout << argv[1] << ": " << au_header_tostr(header) << endl;
    //
    // This example only supports a specific type of AU format:
    //
    // - no additional data in the header
    // - linear PCM encoding
    // - 8KHz sample rate
    // - mono
    //
    if 
    (
        (header.encoding != 2 && header.encoding != 3)
        ||
        header.sample_rate != 8000
        ||
        header.nchannels != 1
    )
    {
        cerr << argv[1] << ": unsupported AU format" << endl;
        return 1;
    }
    
    fin.seekg(header.data_offset);

    const int BUFLEN = 204;

    int16_t sbuf[BUFLEN];
    DtmfDetector detector;
    while (true)
    {
        if (header.encoding == 2) {
          char cbuf[BUFLEN];
          fin.read(cbuf, BUFLEN);
          if (fin.eof()) {
            break;
          }

          // Promote our 8-bit samples to 16 bits, since that's what the detector
          // expects.  Shift them left during promotion, since the decoder won't
          // pick them up otherwise (volume too low).
          //
          for (int j = 0; j < BUFLEN; ++j)
              sbuf[j] = cbuf[j] << 8;

        } else {
          if (fin.eof()) {
            break;
          }
          fin.read((char*)sbuf, BUFLEN * sizeof(int16_t));
          if (swap_endian) {
            for (int j = 0; j < BUFLEN; ++j) {
              swap16_inplace(sbuf[j]);
            }
          }
        }
        
        // detector.zerosIndexDialButton();
        detector.dtmfDetecting(sbuf, BUFLEN);
        cout << detector.getDialButtonsArray() << "'" << endl;
    }

    fin.close();
    return 0;
}
