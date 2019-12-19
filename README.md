# Moaneschien Modules 4 Rack

All still very Alpha.

# Bezosc 

 A Bezier oscillator.

 Manipulate 4 knots and 8 handles of a closed Bezier spline to create waveforms. Each x and y-paramter of the knots and handles can be modulated. Four modes that control the way the handles are atached to the knots.

## Parameters

 x & y of the four knots and x and y of the 8 handles.
 Frequency
 ### Modus

  Mode 1 Rough, knobs and handles are disconnected. Everything moves independently, often resulting in sharp edges and harsher sounds.

  Mode 2 Still rough, but the handles are connected to the knots. So moving a knot up and down wil move the handel up and down in the same manner.

  Mode 3 Semi smooth. One handle is connected to the knot, the other handle lies in the opposite orientation but is scalable. It can have a 'negative' scale to create sharp edges.

  Mode 4 Smooth. One handle is connected to the knot, the other one is calculated for maximum smoothness of the curve 

## Inputs

 Frequecy setting or modulation (V)

## Outputs

 X and y of the resulting shape at t.
 Angle of the direction vector <x,y>
 Length of the direction vector <x,y>
 
 X and y of the tangent vector at t.
 Angle of the tangent vector.
 Length of the tangent vector.

## Use

 The scope is not implemented yet and may never materialize. Use a scope to add x&y to visualise the shape of the spline.

![Bezosc](https://Moaneschien.github.io/modules/images/bezosc_02.png)

# Rndbezosc

 A morphing random Bezier oscilator.

 Two random four segment 1D Bezier splines are created and morphed. When the morph is finished a new random spline morph target is generated. The smoothnes of the wave is controlable to some extent.

## Parameters

 Frequency
 
 Morph steps: Sets how many 'ticks' the morph takes, form 100 to 5000. @44.1 kHz from ca. 2.27 µs (noisy) to  ca. 113 µs (smooth). Defaults @ ca. 45 µs.

 Rough - Smooth: Three steps, maximum smooth wave, a waveform where half of it is smooth and one that is fully random.

## Inputs

 Frequency (V).

## Outputs

 Waveform (V).

![rndbezosc](https://Moaneschien.github.io/modules/images/rndbezosc_03.png)

# Ramp 

 Ramp for VCV Rack. Ramps voltage up or down using cosine, linear or exponential interpolation.

## Parameters

 Start: Requires a trigger as input. Starts the ramp from Vfrom to Vto. A start while running restarts the ramp. When the end is reached voltage stays at Vto and an end puls is emitted. 

 Stop: Requires an trigger as input. Stops/resets the proces, voltage drops to 0 or -5.

 Vfrom: Set starting voltage.

 Vto: Set end voltage.  

 Time: Ramp-time set in seconds, 15 minutes at max.

 Interp: set imterpolation method. At 0 = cosine interpolation, 1 is linear and between 0 and 1 and 1 and 10 is exponential. At 10 it is a step.

 End: Trigger puls that signals reaching the end voltage / time

 Vbi out: -5 - 5V output.

 Vuni out: 0 - 10V output.

![Ramp](https://Moaneschien.github.io/modules/images/ramp.png)

## Credits

 Andrew Belt for VCV RACK, © 2019, GNU General Public License v3.0
  
 Matt McInerney for Orbitron, © 2009, SIL Open Font License, Version 1.1.

 Impallari for Dancing Script, SIL Open Font License, Version 1.1.