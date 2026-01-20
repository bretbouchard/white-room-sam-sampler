#!/usr/bin/env python3
"""
Create a minimal valid SoundFont 2 file for testing
"""
import struct
import os

def create_test_sf2(filename):
    """Create a minimal SoundFont 2 file with simple test tones"""

    # RIFF chunk header
    riff_id = b'RIFF'
    riff_size = 0  # Will update later
    riff_form = b'sfbk'

    # INFO chunk (minimal)
    info_id = b'LIST'
    info_size = 0  # Will update
    info_type = b'INFO'

    # IFIL subchunk (version)
    ifil_id = b'ifil'
    ifil_size = 4
    ifil_version = struct.pack('<HH', 2, 0)  # SF2 version 2.0

    # INAM subchunk (name)
    inam_id = b'INAM'
    inam_name = b'Test Drum Kit\0'
    inam_size = len(inam_name)

    # ISNG subchunk (engine)
    isng_id = b'ISNG'
    isng_name = b'EMU8000\0'
    isng_size = len(isng_name)

    # Calculate INFO chunk size
    info_data = ifil_id + struct.pack('<I', ifil_size) + ifil_version
    info_data += inam_id + struct.pack('<I', inam_size) + inam_name
    info_data += isng_id + struct.pack('<I', isng_size) + isng_name

    # Align to word boundary
    info_size = len(info_data)
    if info_size % 2 != 0:
        info_data += b'\0'
        info_size += 1

    info_size_total = 4 + len(info_data)  # LIST type + data

    # sdta chunk (sample data - minimal test tone)
    sdta_id = b'sdta'
    smpl_id = b'smpl'

    # Create a simple test sample (1 second of 440Hz sine wave)
    import math
    sample_rate = 44100
    duration = 1.0
    num_samples = int(sample_rate * duration)

    # Generate mono test tone
    sample_data = b''
    for i in range(num_samples):
        t = float(i) / sample_rate
        value = int(32767 * 0.5 * math.sin(2 * math.pi * 440 * t))
        sample_data += struct.pack('<h', value)

    smpl_size = len(sample_data)
    sdta_size = 4 + 4 + smpl_size  # smpl id + size + data

    # Align to word boundary
    if smpl_size % 2 != 0:
        sample_data += b'\0'
        smpl_size += 1
        sdta_size += 1

    # pdta chunk (preset data - minimal)
    pdta_id = b'pdta'

    # For simplicity, we'll create minimal preset structures
    # This is a simplified SF2 structure just for testing the pipeline

    # PHDR (preset headers)
    phdr_id = b'phdr'
    # Create one preset
    preset_name = b'Test Drum\0'.ljust(20, b'\0')
    preset = preset_name + struct.pack('<HH', 0, 0)  # preset, bank
    preset += struct.pack('<HHH', 0, 0, 0)  # pbag index, library, genre
    preset += struct.pack('<I', 0)  # reserved
    phdr_data = preset + b'EOP\0' + struct.pack('<HHHHII', 0, 0, 0, 1, 0, 0)
    phdr_size = len(phdr_data)

    # PBAG (preset bags)
    pbag_id = b'pbag'
    pbag_data = struct.pack('<HH', 0, 0)  # gen index, mod index
    pbag_data += struct.pack('<HH', 0, 0)  # EOP
    pbag_size = len(pbag_data)

    # PMOD (preset modulators)
    pmod_id = b'pmod'
    pmod_data = struct.pack<'HHHHHHH', 0, 0, 0, 0, 0, 0, 0)  # Empty modulator
    pmod_data += struct.pack<'HHHHHHH', 0, 0, 0, 0, 0, 0, 0)  # Terminator
    pmod_size = len(pmod_data)

    # PGEN (preset generators)
    pgen_id = b'pgen'
    pgen_data = struct.pack<'HH', 0, 0)  # Empty generator
    pgen_data += struct.pack<'HH', 0, 0)  # Terminator
    pgen_size = len(pgen_data)

    # INST (instruments)
    inst_id = b'inst'
    inst_name = b'Test Inst\0'.ljust(20, b'\0')
    inst_data = inst_name + struct.pack('<HH', 0, 0)  # bag index
    inst_data += b'EOI\0' + struct.pack('<HH', 0, 1)
    inst_size = len(inst_data)

    # IBAG (instrument bags)
    ibag_id = b'ibag'
    ibag_data = struct.pack('<HH', 0, 0)
    ibag_data += struct.pack('<HH', 0, 0)
    ibag_size = len(ibag_data)

    # IMOD (instrument modulators)
    imod_id = b'imod'
    imod_data = struct.pack<'HHHHHHH', 0, 0, 0, 0, 0, 0, 0)
    imod_data += struct.pack<'HHHHHHH', 0, 0, 0, 0, 0, 0, 0)
    imod_size = len(imod_data)

    # IGEN (instrument generators)
    igen_id = b'igen'
    igen_data = struct.pack<'HH', 0, 0)
    igen_data += struct.pack<'HH', 0, 0)
    igen_size = len(igen_data)

    # SHDR (sample headers)
    shdr_id = b'shdr'
    sample_name = b'TestSample\0'.ljust(20, b'\0')
    shdr_data = sample_name
    shdr_data += struct.pack('<I', 0)  # start
    shdr_data += struct.pack('<I', num_samples)  # end
    shdr_data += struct.pack('<I', 0)  # start loop
    shdr_data += struct.pack('<I', 0)  # end loop
    shdr_data += struct.pack('<I', sample_rate)  # sample rate
    shdr_data += struct.pack('<BB', 0, 0)  # original pitch, pitch correction
    shdr_data += struct.pack('<HH', 0, 0)  # sample link, sample type
    shdr_data += b'\0' * 36  # reserved (was 20, should be 36 for SF2.0 spec)
    shdr_data += b'EOS\0' + b'\0' * 17  # + terminator
    shdr_data += struct.pack('<IIIIIIII', 0, 0, 0, 0, 0, 0, 0, 0)  # EOS zeros
    shdr_size = len(shdr_data)

    # Calculate pdta size
    pdta_data = phdr_id + struct.pack('<I', phdr_size) + phdr_data
    pdta_data += pbag_id + struct.pack('<I', pbag_size) + pbag_data
    pdta_data += pmod_id + struct.pack('<I', pmod_size) + pmod_data
    pdta_data += pgen_id + struct.pack('<I', pgen_size) + pgen_data
    pdta_data += inst_id + struct.pack('<I', inst_size) + inst_data
    pdta_data += ibag_id + struct.pack('<I', ibag_size) + ibag_data
    pdta_data += imod_id + struct.pack('<I', imod_size) + imod_data
    pdta_data += igen_id + struct.pack('<I', igen_size) + igen_data
    pdta_data += shdr_id + struct.pack('<I', shdr_size) + shdr_data
    pdta_size = len(pdta_data)

    # Calculate total RIFF size
    total_size = 4 + 4 + 8 + info_size_total + 8 + sdta_size + 8 + pdta_size
    riff_size = total_size - 8  # Exclude RIFF ID and size field

    # Write the file
    with open(filename, 'wb') as f:
        # RIFF header
        f.write(riff_id)
        f.write(struct.pack('<I', riff_size))
        f.write(riff_form)

        # INFO chunk
        f.write(info_id)
        f.write(struct.pack('<I', info_size_total))
        f.write(info_type)
        f.write(info_data)

        # sdta chunk
        f.write(sdta_id)
        f.write(struct.pack('<I', sdta_size))
        f.write(smpl_id)
        f.write(struct.pack('<I', smpl_size))
        f.write(sample_data)

        # pdta chunk
        f.write(pdta_id)
        f.write(struct.pack('<I', pdta_size))
        f.write(pdta_data)

    print(f"Created test SoundFont: {filename}")
    print(f"  Sample rate: {sample_rate} Hz")
    print(f"  Duration: {duration} seconds")
    print(f"  File size: {os.path.getsize(filename)} bytes")

if __name__ == '__main__':
    create_test_sf2('test_drums.sf2')
