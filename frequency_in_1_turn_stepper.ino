+	// Sketch to drive a speedometer with a stepper motor via an input that pulses
+	// a fixed number of times per mile
+	// Copyright (c) 2014  Kevin Gale <kevin@nhwoods.net>
+	// Version 1.0
+	// 
+	// Permission is hereby granted, free of charge, to any person obtaining a copy
+	// of this software and associated documentation files (the "Software"), to deal
+	// in the Software without restriction, including without limitation the rights
+	// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
+	// copies of the Software, and to permit persons to whom the Software is
+	// furnished to do so, subject to the following conditions:
+	// 
+	// The above copyright notice and this permission notice shall be included in
+	// all copies or substantial portions of the Software.
+	// 
+	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
+	// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
+	// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
+	// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
+	// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
+	// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
+	// THE SOFTWARE.
+	//
+	
+	#include "SwitecX25.h"
+	
+	// This code uses the FreqMeasure library written by  Paul Stoffregen <paul@pjrc.com>
+	// http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
+	// However to get a stable speed at high speedometer speeds it will probably be
+	// necessary to increase the size of the FREQMEASURE_BUFFER_LEN value in the
+	// FreqMeasure code.  The FreqMeasure code this software was written against
+	// had a FREQMEASURE_BUFFER_LEN of 12. In order to have a stable speedo value
+	// and 100 mph or above the buffer needed to be at least 20 and a value of 50
+	// seems to work just fine and allows a stable speed.  
+	#include "FreqMeasure.h"
+	
+	const int UpdateInterval = 100;  // 100 milliseconds speedo update rate
+	const double StepsPerDegree = 3.0;  // Motor step is 1/3 of a degree of rotation
+	const unsigned int MaxMotorRotation = 315; // 315 degrees seems like a common safe value.
+	const unsigned int MaxMotorSteps = MaxMotorRotation * StepsPerDegree;
+	const double PulsesPerMile = 5764.0; // Number of input pulses per mile
+	const double SecondsPerHour = 3600.0;
+	
+	// For this speedo 180 degrees is 87 MPH. It is usually easier to measure half the speedo
+	// It's worth noting that although this code uses the terms MPH and miles the code is actually
+	// unit agnostic. If you just plug in kilometers values into the PulsesPerMile and here in the
+	// SpeedoDegreesPerMPH it will work with kilometers.
+	const double SpeedoDegreesPerMPH = 180.0 / 87.0;  
+	
+	unsigned long PreviousMillis = 0;   // last time we updated the speedo
+	double MinMotorStep;  // lowest step that will be used - calculated from update interval
+	
+	SwitecX25 Motor(MaxMotorSteps, 4,5,6,7); // Create the motor object with the maximum steps allowed
+	
+	void setup(void) 
+	{
+	  // Set 8 to pullup. This may or may not be needed depending on the input. The input might come from
+	  // a TTL or active buffer thus driving the input line to both 5V and 0V, or the internal pullup on
+	  // the ATMega processor might not be a small enough value for a particular Hall sensor.  The 
+	  // ATMega328 spec says the internal pullup is  between 20K and 50Kohms.  Some Hall sensors require
+	  // closer to 10K to work correctly on 5V. So turning off the internal pullup is logical sometimes.
+	  pinMode(8, INPUT_PULLUP);
+	
+	  Motor.zero(); //Initialize stepper at 0 location
+	
+	  // The UpdateInterval controls the minimum speed we can measure. Since we force to zero when two
+	  // intervals have gone by without a pulse. To avoid the speedo jumping around at very low speeds we
+	  // just force these low speeds to zero. 
+	  MinMotorStep = PulseToStep(2 * (UpdateInterval / 1000.0) * F_CPU);
+	
+	  FreqMeasure.begin(); // Start freqmeasure library
+	}
+	
+	double sum=0;
+	int count=0;
+	double avgPulseLength=0;
+	unsigned int motorStep = 0;
+	int noInputCount = 0;
+	
+	void loop() {
+	  unsigned long currentMillis = millis();
+	
+	  // Update the motor position every UpdateInterval milliseconds
+	  if (currentMillis - PreviousMillis >= UpdateInterval) {
+	    PreviousMillis = currentMillis;   
+	    count = 0;
+	    sum = 0;
+	
+	    // Read all the pulses available so we can average them
+	    while (FreqMeasure.available()) {
+	      sum += FreqMeasure.read();
+	      count++;
+	    }
+	
+	    if (count) {
+	      // Average all the readings we got over our fixed time interval. This helps
+	      // stabilize the speedo at higher speeds. The pulse length gets shorter and 
+	      // thus harder to measure accurately but we get more pulses to average.
+	      // It may be necessary to update the FreqMeasure library to change the buffer
+	      // length to hold the full number of pulses per update interval at the highest
+	      // speedo values.
+	      avgPulseLength = sum / count;
+	      motorStep = PulseToStep(avgPulseLength);
+	      noInputCount = 0;
+	    } 
+	    else if (++noInputCount == 2)  // force speed to zero after two missed intervals
+	      motorStep = 0;
+	
+	    // Ignore speeds below the the two missed intervals speed so the motor doesn't jump
+	    if (motorStep <= MinMotorStep) 
+	      motorStep = 0;
+	
+	    Motor.setPosition(motorStep); 
+	  }
+	
+	  // Always update the motor. It doesn't instantly go to the desired step so even if
+	  // we didn't call setPosition the motor may still be moving to position from the last
+	  // setPosition call.
+	  Motor.update();
+	}
+	
+	
+	// The FreqMeasure gives us the pulse length in CPU cycles.  This formula converts this into a motor step.
+	// Basically we are converting the length of the pulse in CPU cycles into pulses per second and then
+	// converting that into MPH, Once we have MPH that number is converted into degrees and that is then
+	// converted into a number of steps.
+	unsigned int PulseToStep(double pulseLength)
+	{
+	  return (unsigned int)((F_CPU * SecondsPerHour * SpeedoDegreesPerMPH * StepsPerDegree) / (PulsesPerMile * pulseLength));
+	}
+	
+

