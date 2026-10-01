#include "cdi/common.hpp"

void SCC68070::uart_tx_log_line()
{
	if (UART_T.chars.size() > 0)
	{
		std::string line;
		for (size_t i = 0; i < UART_T.chars.size(); i++)
		{
			if (!std::iscntrl(UART_T.chars[i]))
				line += UART_T.chars[i];
		}
		if (!line.empty())
			MiniCDI::Log("[SCC68070:UART_TX] %s", line.c_str());
	}

	UART_T.chars.clear();
}

void SCC68070::dma_call(size_t index, uint32_t start_address)
{
	if (DMA[index].CCR & 0x80)
	{
		// Indicate that operation is in progress.
		DMA[index].CCR &= ~0x80; // START unset
		DMA[index].CSR &= 0b01001111; // COC, NDT and ERR unset
		DMA[index].CSR |= 0b00001000; // Channel Active set
		DMA[index].CER = 0;

		if (index == 1) start_address = DMA[1].DAC;

		MiniCDI::Log("[SCC68070:DMA%d] $%08X %s $%08X (%d %s)", index+1,
					 start_address,
					 DMA[index].OCR & 0x80 ? "=>" : "<=",
					 DMA[index].MAC,
					 DMA[index].MTC,
					 DMA[index].OCR & 0x10 ? "words" : "bytes"/*,
					 DMA[index].DCR & 0x80 ? "cycle-steal" : "burst"*/);

		// Avoid segmentation fault
		if (DMA[index].MAC >= 8*1024*1024 || start_address >= 8*1024*1024) goto end;

		while (DMA[index].MTC > 0) {
			if (DMA[index].CCR & 0x10) {
				MiniCDI::Log("[SCC68070:DMA%d] transfer aborted", index+1);
				DMA[index].CSR |= 0b10010000; // COC and ERR set
				DMA[index].CSR &= 0b11110111; // Channel Active unset
				DMA[index].CER = 0b10001u; // Abort Error
				return;
			}

			// Set operation based on external request mode (DCR bit 15)
			// Cycle-steal mode (transfer one byte at a time and continue looping to end)
			if (DMA[index].DCR & 0x80)
			{
				// Determine direction (OCR bit 7)
				// If bit set, device to memory
				if (DMA[index].OCR & 0x80)
				{
					memory[DMA[index].MAC] = memory[start_address++];
					if (DMA[index].OCR & 0x10) memory[DMA[index].MAC+1] = memory[start_address++];
				}
				// Otherwise, memory to device.
				else
				{
					memory[start_address++] = memory[DMA[index].MAC];
					if (DMA[index].OCR & 0x10) memory[start_address++] = memory[DMA[index].MAC+1];
				}

				// Increment MAC/DAC registers accordingly and decrement MTC.
				if (index == 1 && (DMA[index].SCR & 0x04)) {
					DMA[index].MAC += DMA[index].OCR & 0x10 ? 2 : 1;
					DMA[index].DAC += DMA[index].OCR & 0x10 ? 2 : 1;
				} else if (index == 0) {
					DMA[index].MAC += DMA[index].OCR & 0x10 ? 2 : 1;
				}

				DMA[index].MTC--;
			}

			// Burst mode (transfer the memory block using memcpy)
			else
			{
				// Determine direction (OCR bit 7), same as before
				if (DMA[index].OCR & 0x80)
					memcpy(&memory[DMA[index].MAC], &memory[start_address], DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC);
				else
					memcpy(&memory[start_address], &memory[DMA[index].MAC], DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC);

				// Increment MAC/DAC registers accordingly and set MTC to 0.
				if (index == 1 && (DMA[index].SCR & 0x04)) {
					DMA[index].MAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
					DMA[index].DAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
				} else if (index == 0) {
					DMA[index].MAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
				}

				DMA[index].MTC = 0;
			}
		}

		end:
		// Indicate that operation is completed.
		DMA[index].CSR |= 0b10000000; // COC set
		DMA[index].CSR &= 0b11110111; // Channel Active unset
		interrupt(index == 1 ? SCC68070::IPL_DMA2 : SCC68070::IPL_DMA1, true);
	}
}