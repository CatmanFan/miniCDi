#ifndef MINICDI_ADPCMDECODER
#define MINICDI_ADPCMDECODER

/*****
  DISCLAIMER:
  Partially sourced from ogarvey's https://github.com/ogarvey/OGLibCD-i and algorithm used in Stovent's CeDImu.
 *****/

class AdpcmDecoder
{
	int K0[4] = { 0, 240, 460, 392 };
	int K1[4] = { 0, 0, -208, -220 };

	int lk0 = 0;
	int rk0 = 0;
	int lk1 = 0;
	int rk1 = 0;

	uint8_t ranges[8];
	uint8_t filters[8];
	int8_t sound_data[8][28]; // A: 4 sound units, BC: 8 sound units -- 28 sound data bytes

	template <size_t max_units, int gain>
	void decode_adpcm(bool stereo, bool low_freq);

public:
	enum SoundQualityLevel
	{
		NULL_SQL = 0,
		CDDA_SQL, // sampling frequency: 44100 Hz ; bps: 16bps ; bandwidth: 20 kHz (stereo only)
		CDI_A, // sampling frequency: 37800 Hz ; bps: 8bps ; bandwidth: 17 kHz
		CDI_B, // sampling frequency: 37800 Hz ; bps: 4bps ; bandwidth: 17 kHz
		CDI_C  // sampling frequency: 18900 Hz ; bps: 4bps ; bandwidth: 8.5 kHz
	};

	std::vector<int16_t> left, right;
	int16_t output[16128]; // 8*28 * 2 (18.9 kHz) * 2 (stereo) * 18 (sound groups)
	int output_size = 0;

	/**
	 * @brief  Decodes a CD-i audio sector to a 16-bit sample buffer.
	 *
	 * @param  buffer:  pointer to the sector buffer, including header
	 *
	 * @return Whether the sector is valid or not.
	 */
	bool decode_sector(uint8_t *buffer);
};

#endif