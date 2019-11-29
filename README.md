# Ramp (very alpha)
 Ramp for VCV Rack. Ramps voltage up or down using cosine, linear or exponential interpolation.

 Start: Requires an trigger as input. starts the ramp from Vfrom to Vto. A start while running restarts the ramp. When the end is reached voltage stays at Vto and an end puls is emitted. 

 Stop: Requires an trigger as input. Stops/resets the proces, voltage drops to 0 or -5.

 Vfrom: Set starting voltage.

 Vto: Set end voltage.  

 Time: Ramp-time set in seconds, 15 minutes at max.

 Interp: set imterpolation method. At 0 = cosine interpolation, 1 is linear and between 0 and 1 and 1 and 10 is exponential. At 10 it is a step.

 End: Trigger puls that signals reaching the end voltage / time

 Vbi out: -5 - 5V output.

 Vuni out: 0 - 10V output.

