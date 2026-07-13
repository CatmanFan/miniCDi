#ifndef MINICDI_ADPCMDECODER
#define MINICDI_ADPCMDECODER

class AdpcmDecoder
{
public:
	enum SoundQualityLevel
	{
		NULL_SQL = 0,
		CDDA_SQL,
		CDI_A, // sampling frequency: 37800 Hz ; bps: 8bps ; bandwidth: 17000 Hz,
		CDI_B,
		CDI_C
	};

	/**
	 * @brief  Decodes a CD-i audio sector to a 16-bit sample buffer.
	 *
	 * @param  coding:  The coding byte as obtained from the sector.
	 * @param  src:     pointer to the sector buffer
	 * @param  dst:     destination std::vector
	 *
	 * @return Whether the sector is valid or not.
	 */
	inline bool decode(uint8_t coding, uint8_t *src, std::vector<int16_t> &dst)
	{
		if (!coding) return false;

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
			uint8_t F = *(src + (i*128)) >> 4 & 0x0F, R = *(src + (i*128)) & 0x0F;
			switch (level)
			{
				default:
					break;

				case CDI_B:
				case CDI_C:
					MiniCDI::Log("[Audio] CD-i level %s detected", level == CDI_C ? "C" : "B");
					break;
			}
		}
		// ***************************

		return true;
	}
};

#endif