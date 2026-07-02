#pragma once

#include <cstdint>
#include <vector>
#include "ymfm_opn.h"

// Native port of cpp/opna_wrapper.cpp from the opna web app (de-globalized).
// Same chip setup as the WASM build: ymfm::ym2608, OPN_FIDELITY_MED, 8MHz clock,
// SSG mixed into both channels at -3dB (x707/1000).
class OpnaChip : private ymfm::ymfm_interface
{
public:
    static constexpr uint32_t kClock = 8000000;

    OpnaChip() : chip (*this)
    {
        chip.set_fidelity (ymfm::OPN_FIDELITY_MED);
        chip.reset();
    }

    void reset() { chip.reset(); }

    uint32_t sampleRate() const { return const_cast<ymfm::ym2608&> (chip).sample_rate (kClock); }

    void write (uint32_t port, uint8_t addr, uint8_t data)
    {
        if (port == 0)
        {
            chip.write_address (addr);
            chip.write_data (data);
        }
        else
        {
            chip.write_address_hi (addr);
            chip.write_data_hi (data);
        }
    }

    // Generate one native-rate frame. Same mix as opna_wrapper.cpp ym2608_generate.
    void generate (int32_t& left, int32_t& right)
    {
        ymfm::ym2608::output_data output;
        chip.generate (&output, 1);
        const int32_t ssg = output.data[2] * 707 / 1000;
        left  = output.data[0] + ssg;
        right = output.data[1] + ssg;
    }

    void loadRhythmRom (const uint8_t* data, size_t size) { adpcmARom.assign (data, data + size); }

    void loadAdpcmB (std::vector<uint8_t> data) { adpcmBRam = std::move (data); }
    bool hasAdpcmB() const { return ! adpcmBRam.empty(); }

private:
    uint8_t ymfm_external_read (ymfm::access_class type, uint32_t address) override
    {
        if (type == ymfm::ACCESS_ADPCM_A && address < adpcmARom.size())
            return adpcmARom[address];
        if (type == ymfm::ACCESS_ADPCM_B && address < adpcmBRam.size())
            return adpcmBRam[address];
        return 0;
    }

    void ymfm_external_write (ymfm::access_class type, uint32_t address, uint8_t data) override
    {
        if (type == ymfm::ACCESS_ADPCM_B && address < adpcmBRam.size())
            adpcmBRam[address] = data;
    }

    ymfm::ym2608 chip;
    std::vector<uint8_t> adpcmARom;
    std::vector<uint8_t> adpcmBRam;
};
