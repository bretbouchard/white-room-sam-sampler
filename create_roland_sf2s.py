#!/usr/bin/env python3
"""
Create minimal SoundFont files for Roland drum machines.
These are test files that provide basic drum sounds until real samples can be created.
"""

import struct
import os

def create_sf2(filename, instrument_name, samples_description):
    """
    Create a minimal SoundFont 2.0 file with basic drum sounds.

    Args:
        filename: Output SF2 filename
        instrument_name: Name of the drum machine
        samples_description: Description of the samples
    """

    # RIFF chunk
    riff_id = b'RIFF'
    riff_size = 0  # Will update later
    riff_form = b'sfbk'

    # INFO chunk
    info_id = b'LIST'
    info_size = 0  # Will update later
    info_type = b'INFO'

    # INAM (instrument name)
    inam_id = b'INAM'
    inam_size = len(instrument_name) + 1
    inam_data = instrument_name.encode('utf-8') + b'\x00'

    # IENG (engineer)
    ieng_id = b'IENG'
    ieng_size = len('White Room Audio') + 1
    ieng_data = b'White Room Audio\x00'

    # ISFT (software)
    isft_id = b'ISFT'
    isft_size = len('White Room Sam Sampler') + 1
    isft_data = b'White Room Sam Sampler\x00'

    # ICMT (comment)
    icmt_id = b'ICMT'
    icmt_data = f'Test SoundFont for {instrument_name}. {samples_description}'.encode('utf-8') + b'\x00'
    icmt_size = len(icmt_data)

    # INFO chunk data
    info_data = (
        inam_id + struct.pack('<I', inam_size) + inam_data +
        ieng_id + struct.pack('<I', ieng_size) + ieng_data +
        isft_id + struct.pack('<I', isft_size) + isft_data +
        icmt_id + struct.pack('<I', icmt_size) + icmt_data
    )
    info_size = len(info_data) + 4  # +4 for info_type

    # sdta chunk (sample data - minimal mono sine waves for drums)
    sdta_id = b'LIST'
    sdta_size = 0
    sdta_type = b'sdta'

    # smpl chunk (actual sample data)
    smpl_id = b'smpl'
    smpl_size = 0

    # Create minimal drum samples (simple sine wave bursts)
    sample_rate = 44100
    all_samples = b''

    # Generate simple drum-like sounds
    # Kick: low frequency sine wave with fast decay
    kick_freq = 60
    kick_duration = 0.5  # seconds
    kick_samples = int(kick_duration * sample_rate)
    kick_data = b''
    for i in range(kick_samples):
        t = i / sample_rate
        decay = (1 - (i / kick_samples)) ** 2
        value = int(32767 * decay * 0.8 * (2 ** 15 - 1))
        kick_data += struct.pack('<h', value)
    all_samples += kick_data

    # Snare: noise burst with mid frequency
    snare_duration = 0.3
    snare_samples = int(snare_duration * sample_rate)
    snare_data = b''
    for i in range(snare_samples):
        decay = (1 - (i / snare_samples)) ** 1.5
        value = int(32767 * decay * 0.5 * ((-1) ** (i % 3)))  # Simple noise
        snare_data += struct.pack('<h', value)
    all_samples += snare_data

    # Hi-hat: high frequency noise with fast decay
    hihat_duration = 0.1
    hihat_samples = int(hihat_duration * sample_rate)
    hihat_data = b''
    for i in range(hihat_samples):
        decay = (1 - (i / hihat_samples)) ** 3
        value = int(32767 * decay * 0.3 * ((-1) ** (i % 2)))
        hihat_data += struct.pack('<h', value)
    all_samples += hihat_data

    smpl_data = all_samples
    smpl_size = len(smpl_data)

    sdta_data = smpl_id + struct.pack('<I', smpl_size) + smpl_data
    sdta_size = len(sdta_data) + 4  # +4 for sdta_type

    # pdta chunk (preset data - minimal)
    pdta_id = b'LIST'
    pdta_size = 0
    pdta_type = b'pdta'

    # Create minimal preset and instrument zones
    # PHDR (preset header)
    phdr_id = b'PHDR'
    phdr_size = 0
    phdr_data = b''

    # Default preset
    preset_name = instrument_name[:20].ljust(20, '\x00').encode('utf-8')
    preset_data = (
        preset_name +           # Name
        struct.pack('<H', 0) +  # Preset number
        struct.pack('<H', 0) +  # Bank
        struct.pack('<H', 0) +  # Preset bag index
        struct.pack('<H', 1) +  # Library (not used)
        struct.pack('<I', 0) +  # Genre (not used)
        struct.pack('<I', 0) +  # Morphology (not used)
    )
    phdr_data += preset_data

    # EOP (End of Preset)
    eop_name = b'EOP' + b'\x00' * 17
    eop_data = (
        eop_name +
        struct.pack('<H', 0) +
        struct.pack('<H', 0) +
        struct.pack('<H', 1) +  # Last preset bag index
        struct.pack('<H', 0) +
        struct.pack('<I', 0) +
        struct.pack('<I', 0)
    )
    phdr_data += eop_data
    phdr_size = len(phdr_data)

    # PBAG (preset bag)
    pbag_id = b'PBAG'
    pbag_size = 0
    pbag_data = (
        struct.pack('<H', 0) +  # Generator index
        struct.pack('<H', 0)    # Modulator index
    )
    pbag_size = len(pbag_data)

    # PGEN (preset generator)
    pgen_id = b'PGEN'
    pgen_size = 0
    pgen_data = (
        struct.pack('<H', 41) +     # instrument
        struct.pack('<H', 0) +      # Amount
        b'\x00' * 4                 # Unused
    )
    pgen_size = len(pgen_data)

    # INST (instrument)
    inst_id = b'INST'
    inst_size = 0
    inst_data = b''

    # Default instrument
    inst_name = instrument_name[:20].ljust(20, '\x00').encode('utf-8')
    inst_data = (
        inst_name +
        struct.pack('<H', 0) +  # Bag index
    )
    inst_data += inst_data  # Add twice for start and end

    inst_size = len(inst_data)

    # IBAG (instrument bag)
    ibag_id = b'IBAG'
    ibag_size = 0
    ibag_data = (
        struct.pack('<H', 0) +  # Generator index
        struct.pack('<H', 0)    # Modulator index
    )
    ibag_size = len(ibag_data)

    # IGEN (instrument generator)
    igen_id = b'IGEN'
    igen_size = 0
    igen_data = b''

    # Add sample generators for kick, snare, hi-hat
    sample_start = 0
    kick_end = kick_samples - 1
    snare_start = kick_samples
    snare_end = snare_start + snare_samples - 1
    hihat_start = snare_end + 1
    hihat_end = hihat_start + hihat_samples - 1

    # Kick (note 36)
    igen_data += (
        struct.pack('<H', 5) +      # keyRange
        struct.pack('<H', 36) +     # Low
        struct.pack('<H', 36)       # High
    )
    igen_data += (
        struct.pack('<H', 43) +     # sampleID
        struct.pack('<H', 0) +      # Sample 0 (kick)
        b'\x00' * 4
    )

    # Snare (note 38)
    igen_data += (
        struct.pack('<H', 5) +      # keyRange
        struct.pack('<H', 38) +     # Low
        struct.pack('<H', 38)       # High
    )
    igen_data += (
        struct.pack('<H', 43) +     # sampleID
        struct.pack('<H', 1) +      # Sample 1 (snare)
        b'\x00' * 4
    )

    # Hi-hat (note 42)
    igen_data += (
        struct.pack('<H', 5) +      # keyRange
        struct.pack('<H', 42) +     # Low
        struct.pack('<H', 42)       # High
    )
    igen_data += (
        struct.pack('<H', 43) +     # sampleID
        struct.pack('<H', 2) +      # Sample 2 (hi-hat)
        b'\x00' * 4
    )

    igen_size = len(igen_data)

    # SHDR (sample header)
    shdr_id = b'SHDR'
    shdr_size = 0
    shdr_data = b''

    # Kick sample header
    shdr_data += (
        b'Kick\x00' + b'\x00' * 16 +  # Sample name (20 chars)
        struct.pack('<I', sample_start) +     # Start
        struct.pack('<I', kick_end) +         # End
        struct.pack('<I', 0) +                # Start loop
        struct.pack('<I', 0) +                # End loop
        struct.pack('<I', sample_rate) +      # Sample rate
        struct.pack('<B', 0) +                # Original pitch
        struct.pack('<B', 0) +                # Pitch correction
        struct.pack('<H', 1) +                # Sample link (mono)
        struct.pack('<H', 1)                  # Sample type (ROM)
    )

    # Snare sample header
    shdr_data += (
        b'Snare\x00' + b'\x00' * 15 +
        struct.pack('<I', snare_start) +
        struct.pack('<I', snare_end) +
        struct.pack('<I', 0) +
        struct.pack('<I', 0) +
        struct.pack('<I', sample_rate) +
        struct.pack('<B', 0) +
        struct.pack('<B', 0) +
        struct.pack('<H', 1) +
        struct.pack('<H', 1)
    )

    # Hi-hat sample header
    shdr_data += (
        b'HiHat\x00' + b'\x00' * 15 +
        struct.pack('<I', hihat_start) +
        struct.pack('<I', hihat_end) +
        struct.pack('<I', 0) +
        struct.pack('<I', 0) +
        struct.pack('<I', sample_rate) +
        struct.pack('<B', 0) +
        struct.pack('<B', 0) +
        struct.pack('<H', 1) +
        struct.pack('<H', 1)
    )

    # EOI (End of Instrument)
    shdr_data += (
        b'EOS\x00' + b'\x00' * 17 +
        struct.pack('<I', 0) +
        struct.pack('<I', 0) +
        struct.pack('<I', 0) +
        struct.pack('<I', 0) +
        struct.pack('<I', 0) +
        struct.pack('<B', 0) +
        struct.pack('<B', 0) +
        struct.pack('<H', 0) +
        struct.pack('<H', 0)
    )

    shdr_size = len(shdr_data)

    pdta_data = (
        pdta_type +
        phdr_id + struct.pack('<I', phdr_size) + phdr_data +
        pbag_id + struct.pack('<I', pbag_size) + pbag_data +
        pgen_id + struct.pack('<I', pgen_size) + pgen_data +
        inst_id + struct.pack('<I', inst_size) + inst_data +
        ibag_id + struct.pack('<I', ibag_size) + ibag_data +
        igen_id + struct.pack('<I', igen_size) + igen_data +
        shdr_id + struct.pack('<I', shdr_size) + shdr_data
    )
    pdta_size = len(pdta_data)

    # Update RIFF size
    total_size = (
        4 +  # RIFF form type
        8 + len(info_data) +  # INFO chunk
        8 + len(sdta_data) +  # sdta chunk
        8 + len(pdta_data)    # pdta chunk
    )

    # Write to file
    with open(filename, 'wb') as f:
        # RIFF header
        f.write(riff_id)
        f.write(struct.pack('<I', total_size))
        f.write(riff_form)

        # INFO chunk
        f.write(info_id)
        f.write(struct.pack('<I', info_size))
        f.write(info_type)
        f.write(info_data)

        # sdta chunk
        f.write(sdta_id)
        f.write(struct.pack('<I', sdta_size))
        f.write(sdta_data)

        # pdta chunk
        f.write(pdta_id)
        f.write(struct.pack('<I', pdta_size))
        f.write(pdta_data)

    print(f"✅ Created {filename} ({os.path.getsize(filename)} bytes)")

def main():
    """Create minimal SoundFont files for Roland drum machines."""

    print("Creating minimal SoundFont files for Roland drum machines...")
    print("These are test files until real samples can be created.\n")

    # Create output directory
    output_dir = 'sf2'
    os.makedirs(output_dir, exist_ok=True)

    # Roland TR-909 (26 samples planned)
    create_sf2(
        f'{output_dir}/roland_tr909.sf2',
        'Roland TR-909',
        'Test file - 26 samples planned. Minimal kick/snare/hi-hat included.'
    )

    # Roland TR-606 (49 samples planned)
    create_sf2(
        f'{output_dir}/roland_tr606.sf2',
        'Roland TR-606',
        'Test file - 49 samples planned. Minimal kick/snare/hi-hat included.'
    )

    # Roland TR-707 (66 samples planned)
    create_sf2(
        f'{output_dir}/roland_tr707.sf2',
        'Roland TR-707',
        'Test file - 66 samples planned. Minimal kick/snare/hi-hat included.'
    )

    # Roland TR-505 (14 samples planned)
    create_sf2(
        f'{output_dir}/roland_tr505.sf2',
        'Roland TR-505',
        'Test file - 14 samples planned. Minimal kick/snare/hi-hat included.'
    )

    # Roland TR-626 (34 samples planned)
    create_sf2(
        f'{output_dir}/roland_tr626.sf2',
        'Roland TR-626',
        'Test file - 34 samples planned. Minimal kick/snare/hi-hat included.'
    )

    # Roland CR-78 (78 samples planned)
    create_sf2(
        f'{output_dir}/roland_cr78.sf2',
        'Roland CR-78',
        'Test file - 78 samples planned. Minimal kick/snare/hi-hat included.'
    )

    # Waldorf Blofeld (108 samples planned)
    create_sf2(
        f'{output_dir}/waldorf_blofeld.sf2',
        'Waldorf Blofeld',
        'Test file - 108 samples planned. Minimal kick/snare/hi-hat included.'
    )

    print("\n✅ All test SoundFont files created!")
    print(f"\nLocation: {os.path.abspath(output_dir)}/")
    print("\n⚠️  NOTE: These are minimal test files.")
    print("Real sample files need to be recorded/organized to create full SoundFonts.")
    print("\nGM Standard Mapping:")
    print("  Note 36 (C1) = Kick")
    print("  Note 38 (D1) = Snare")
    print("  Note 42 (F#1) = Closed Hi-Hat")

if __name__ == '__main__':
    main()
