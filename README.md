# midikontrollaz

A library of recreated MIDI controllers for ease of mapping in VCV Rack.

## Modules

### M-Vave SMC Pad

A 1:1 recreation of the M-Vave SMC Pad MIDI controller as a CV output module for VCV Rack.

**Features:**
- 8 rotary knobs outputting CV signals (labeled RATE, SWING, TEMPO, LATCH, VELOCITY 1/2, TRANSPOSE/TRANSPOSE+)
- 16 pads outputting Gate or Trigger signals (configurable per-pad)
- Configurable knob CV ranges: 0-10V, -5V to 5V, 0-5V, 0-1V
- Dark charcoal body with blue LED ring knob indicators, matching the original hardware aesthetic

Located in [`mvave_smc_pad/`](mvave_smc_pad/).
