# gsr+heartbeat+temperature
 Use arduino to collect and process the data from temperature sensor(LM35) + GSR sensor(CJMCU-6701) + Pulse sensor(XD-58C)

Code Flow Description
1. Pulse Sensor 
 - Collect data direct from the sensor (raw data)
 - Process the raw data using the ema calculation to reduce the noise (smooth data)
 - Detect the heartbeat by detect the high increment signal (processed data)
 - Lastly smoothing the data processed to get a more consistant result. (final output)
2. GSR Sensor
 - Collect the data direct from the sensor (raw data)
 - Process the raw data using the ema calculation to reducce the noise (smooth data)
 - by using the ema result can detect the maximum and minimum of every triggered signal (proccessed data)
3. Tempeature
 - Collect and use the data direct from the sensor.
