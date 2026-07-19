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

	inline void decode_adpcm(int max_units, int gain, int frequency, bool stereo)
	{
		for (int sample_unit = 0; sample_unit < max_units; sample_unit++)
		{
			const uint16_t cur_gain = 2 << (gain - ranges[sample_unit]);
			for (uint8_t sample_data = 0; sample_data < 28; sample_data++)
			{
				if (stereo && (sample_unit & 1))
				{
					const int16_t sample = std::clamp((sound_data[sample_unit][sample_data] * cur_gain)
													+ ((rk0 * K0[filters[sample_unit]] + rk1 * K1[filters[sample_unit]]) / 256), INT16_MIN, INT16_MAX);
					rk1 = rk0;
					rk0 = sample;
					right.push_back(sample);
				}
				else
				{
					const int16_t sample = std::clamp((sound_data[sample_unit][sample_data] * cur_gain)
													+ ((lk0 * K0[filters[sample_unit]] + lk1 * K1[filters[sample_unit]]) / 256), INT16_MIN, INT16_MAX);
					lk1 = lk0;
					lk0 = sample;
					left.push_back(sample);
				}
			}
		}
	}

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

	/**
	 * @brief  Decodes a CD-i audio sector to a 16-bit sample buffer.
	 *
	 * @param  coding:  The coding byte as obtained from the sector.
	 * @param  src:     pointer to the sector buffer
	 * @param  dst:     destination std::vector
	 *
	 * @return Whether the sector is valid or not.
	 */
	inline bool decode_sector(uint8_t coding, uint8_t *src)
	{
		// if (!coding) return false;
		// if (coding & 0b10'10'10) return false; // Skip if any set to reserved

		left.clear();
		right.clear();

		int sample_bits = (coding & 0b01'00'00) ? 8 : 4;
		int sample_freq = (coding & 0b00'01'00) ? 18900 : 37800;
		int sample_chan = (coding & 0b00'00'01) ? 2 : 1;
		bool stereo = sample_chan == 2;
		enum SoundQualityLevel level = sample_freq != 37800 ? CDI_C : sample_bits == 8 ? CDI_A : CDI_B;
		// MiniCDI::Log("[Audio:ADPCM] bps: %d, freq: %d, stereo: %d", sample_bits, sample_freq, stereo);
		// bool emphasis = coding >> 6 & 0b01;
		// uint16_t num_samples = 8 >> ((sample_bits == 8) + (sample_chan == 2));

		// We are currently ONLY working with CD-i (not CD-DA) sectors (see CDiDisc.hpp).
		// ***************************
		/// Green Book IV.3:
		/// Audio sectors comprise 18 "sound groups" of size 128 bytes.
		/// Each "sound group" is divided into 16 parameter bytes and sampled audio data.
		memset(sound_data, 0, sizeof(sound_data));
		memset(ranges, 0, sizeof(ranges));
		memset(filters, 0, sizeof(filters));
		for (size_t SG = 0; SG < 18; SG++)
		{
			switch (level)
			{
				default:
					continue;

				case CDI_A:
				{
					for (int i = 0; i < 4; i++) {
						ranges[i] = (uint8_t)(src[(SG*128)+i] & 0x0F);
						filters[i] = (uint8_t)(src[(SG*128)+i] >> 4 & 0x0F);
					}

					uint8_t index = 16;
					for (uint8_t sample_data = 0; sample_data < 28; sample_data++) {
						for (uint8_t sample_unit = 0; sample_unit < 4; sample_unit++) {
							sound_data[sample_unit][sample_data] = src[(SG*128)+index];
							index++;
						}
					}

					decode_adpcm(4, 8, sample_freq, stereo);
				}
				break;

				case CDI_B:
				case CDI_C:
				{
					for (int i = 0; i < 8; i++)
					{
						ranges[i] = (uint8_t)(src[(SG*128)+i+4] & 0x0F);
						filters[i] = (uint8_t)(src[(SG*128)+i+4] >> 4 & 0x0F);
					}

					uint8_t index = 16;
					for (uint8_t sample_data = 0; sample_data < 28; sample_data++) {
						for (uint8_t sample_unit = 0; sample_unit < 8;) {
							const uint8_t SB = src[(SG*128)+index];
							index++;

							int8_t SD0 = SB & 0x0F;
							if (SD0 >= 8) SD0 -= 16;

							int8_t SD1 = SB >> 4 & 0x0F;
							if (SD1 >= 8) SD1 -= 16;

							sound_data[sample_unit++][sample_data] = SD0;
							sound_data[sample_unit++][sample_data] = SD1;
						}
					}

					decode_adpcm(8, 12, sample_freq, stereo);
				}
				break;
			}
		}
		// ***************************

		return true;
	}
};

#endif