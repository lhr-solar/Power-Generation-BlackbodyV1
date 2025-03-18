# PowerGenerationBlackbody
Gathers irradiance and temperature data for telemetry and MPPT algorithmic input.

**Blackbody A:**
Central board. Gathers irradiance and temperature data from multiple Blackbody Cs and sends it to CarCAN.


**Blackbody C:**
Can daisy-chain multiple Blackbody Cs to one blackbody A. Receives data from an irradiance sensor and thermocouple and sends it to Blackbdoy A through I2C. Attached beneath the top shell. 


**Sensor Board:**
Holds the irradiance sensor. Placed on the top shell between solar modules and connected with wires to blackbody C. 


