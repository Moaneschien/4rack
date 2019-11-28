# Ramp (very alpha)
 Ramp for cvc rack. Ramps voltage using cosine, linear or exponential interpolation.

 Start trigger starts the ramp from Vfrom to Vto. When the end is reached voltage stays at Vto and an end puls is emitted. 

 Stop stops/resets the proces. A start while running restarts the ramp. 

 Interp set at 0 = cosine interpolation, 1 is linear and between 0 and 1 and 1 and 10 is exponential. At 10 it is a step. 

 Time is set in seconds, 15 at max.
