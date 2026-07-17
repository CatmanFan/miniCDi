#ifndef MINICDI_ADPCMDECODER
#define MINICDI_ADPCMDECODER

class AdpcmDecoder
{
    int K0[4] = { 0, 240, 460, 392 };
    int K1[4] = { 0, 0, -208, -220 };

    int lk0 = 0;
    int rk0 = 0;
    int lk1 = 0;
	int rk1 = 0;

	uint8_t range[8];
	uint8_t filter[8];
	int8_t SD[8][28];

    inline uint8_t decode_adpcm(int su, int gain, bool stereo)
    {
		uint8_t index = 0;

		for (int i = 0; i < su; i++)
		{
			uint16_t curGain = (uint16_t)(2 << (gain - range[i]));
			for (uint8_t ss = 0; ss < 28; ss++)
			{
				if (stereo && (i & 1) == 1)
				{
					int32_t sample = std::clamp((SD[i][ss] * curGain) + ((rk0 * K0[filter[i]] + rk1 * K1[filter[i]]) / 256), INT16_MIN, INT16_MAX);
					rk1 = rk0;
					rk0 = sample;
					right.push_back(sample);
					index++;
				}
				else
				{
					int32_t sample = std::clamp((SD[i][ss] * curGain) + ((lk0 * K0[filter[i]] + lk1 * K1[filter[i]]) / 256), INT16_MIN, INT16_MAX);
					lk1 = lk0;
					lk0 = sample;
					left.push_back(sample);
					index++;
				}
			}
		}

		return index;
    }

public:
	enum SoundQualityLevel
	{
		NULL_SQL = 0,
		CDDA_SQL,
		CDI_A, // sampling frequency: 37800 Hz ; bps: 8bps ; bandwidth: 17000 Hz,
		CDI_B,
		CDI_C
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
		if (!coding) return false;
		left.clear();
		right.clear();

		// Skip if any set to reserved
		if ((coding >> 4 & 0b11) >= 2 || (coding >> 2 & 0b11) >= 2 || (coding & 0b11) >= 2)
			return false;

		enum SoundQualityLevel level = (coding & 0x10) ? CDI_A : (coding & 0x40) ? CDI_C : CDI_B;

		int sample_bits = level == CDI_A ? 8 : 4;
		int sample_freq = level == CDI_C ? 18900 : 37800;
		int sample_chan = (coding & 0x01) ? 2 : 1;
		bool emphasis = coding >> 6 & 0b01;
		uint16_t num_samples = 8 >> ((sample_bits == 8) + (sample_chan == 2));

		// We are currently ONLY working with CD-i (not CD-DA) sectors (see CDiDisc.hpp).
		// ***************************
		/// Green Book IV.3:
		/// Audio sectors comprise 18 "sound groups" of size 128 bytes.
		/// Each "sound group" is divided into 16 parameter bytes and sampled audio data.
		for (int i = 0; i < 18; i++)
		{
			switch (level)
			{
				default:
					return false;

				case CDI_A:
				{
					uint8_t index = 16 + i*128;
					memset(range, 0, sizeof(range));
					memset(filter, 0, sizeof(filter));
					memset(SD, 0, sizeof(SD));

					for (int i = 0; i < 8; i++)
					{
						range[i] = (uint8_t)(*(src+index) & 0x0F);
						filter[i] = (uint8_t)(*(src+index) >> 4);
					}

					for (uint8_t ss = 0; ss < 28; ss++) // sound sample
					{
						for (uint8_t su = 0; su < 4; su++) // sound unit
						{
							SD[su][ss] = (int8_t)(*(src+index));
							index++;
						}
					}

					index = decode_adpcm(4, 8, sample_chan == 2);
				}
				break;

				case CDI_B:
				case CDI_C:
				{
					uint8_t index = 4 + i*128;
					memset(range, 0, sizeof(range));
					memset(filter, 0, sizeof(filter));
					memset(SD, 0, sizeof(SD));

					for (int i = 0; i < 8; i++)
					{
						range[i] = (uint8_t)(*(src+index) & 0x0F);
						filter[i] = (uint8_t)(*(src+index) >> 4);
					}

					index = 16 + i*128;
					for (uint8_t ss = 0; ss < 28; ss++)
					{
						for (uint8_t su = 0; su < 8; su += 2)
						{
							uint8_t SB = *(src+index);
							SD[su][ss] = (int8_t)(SB & 0x0F);
							if (SD[su][ss] >= 8) SD[su][ss] -= 16;
							SD[su + 1][ss] = (int8_t)(SB >> 4);
							if (SD[su + 1][ss] >= 8) SD[su + 1][ss] -= 16;
							index++;
						}
					}

					index = decode_adpcm(8, 12, sample_chan == 2);
				}
				break;
			}
		}
		// ***************************

		return true;
	}
};

#endif