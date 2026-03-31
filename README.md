# Making-an-accurate-handheld-dB-meter
A small handheld and very accurate sound meter to measure SNR or noise ambiance, the DB meter.
[View Project on Hackster](https://www.hackster.io/electroboy001/making-an-accurate-handheld-db-meter-70314f)

Because I am a sound lover, and in recent times I have done a lot of research on audio-related products. But I am always curious to measure the sound performance in terms of dB(A). In that way, when the amplifier is in steady state, I can measure the total output noise due to the surroundings. Because decibels are a logarithmic scale, it usually starts from a very light value, and it can also measure very high values. I have many options, as someone on the internet says, there are phone apps that claim to measure sound levels, but let's be honest, a phone microphone is designed for voice calls, not calibrated acoustic measurement. 

![Image](https://github.com/user-attachments/assets/d11b1ffc-ad36-4dc3-96e5-7531eda54738)

So if you want to measure actual sound output, there are professional sound level meters, but they start at a couple of hundred dollars. But I wanted something in between a handheld dB meter that's actually accurate and battery-friendly. When I was searching the web, I came across the DFRobot Gravity Analog Sound Level Meter (SEN0232). This little board has a professional-grade MEMS microphone with proper signal conditioning. It has an A-weighting filter and a beautifully simple analog output. The voltage coming out is linearly proportional to decibels. So, no complex DSP, no FFT, just read an analog pin and multiply. I paired it with my [own designed Arduino from JLCPCB](https://jlcpcb.com/?from=audrey3) and a 0.96" OLED display, and that’s how I got myself a proper handheld dB meter.

What is a Sound Level Meter?

Before jumping into the process of building the meter first let’s what actually is dBA and dB. Human ears can hear sounds between 1kHz and 5kHz, and are less sensitive to the other ranges. A-weighting is a frequency response curve that adjusts the measurement to match human perception. Here is the representation over frequency you can see the plots:

![Image](https://github.com/user-attachments/assets/204142bf-50d9-43da-9742-2358efc93e04)

When you see dBA, that's a decibel reading with A-weighting applied. This is important because the SEN0232 uses A-weighting, which means the readings represent how loud something sounds to you, not just the raw acoustic energy. That's exactly what you want for a practical sound level meter. Here is the dBA reference scale:

<img width="759" height="326" alt="Image" src="https://github.com/user-attachments/assets/77afbcae-b7eb-42da-a821-538e87e10e67" />

DFRobot Gravity Analog Sound Level Meter:

The SEN0232 is not just a microphone breakout board, but it's a complete sound level measurement module with an onboard signal conditioning circuit. [DFRobot Analog Sound Level Meter](https://www.dfrobot.com/product-1663.html?srsltid=AfmBOopiAB0wIjJjKp2_8b-FmOPcTs_SFuejP5NUATcJ6zYNgUqGxCbx)

![Image](https://github.com/user-attachments/assets/04d2fa74-30a4-4482-8a57-e13de4df9125)

Key Specifications:

Measurement Range: 30 dBA to 130 dBA

Measurement Error: ±1.5 dB

Frequency Weighting: A-Weighted

Frequency Response: 31.5 Hz to 8.5 kHz

Time Characteristic: 125 ms (Fast mode)

Input Voltage: 3.3V to 5.0V

Input Current: 22 mA @ 3.3V / 14 mA @ 5.0V

Output Voltage: 0.6V to 2.6V (Analog)

The board comes with a sensitivity MEMS microphone for capturing sound pressure waves, then followed by an A-Weighting Filter that shapes the frequency response to match human hearing. Some signal conditioning and rectification circuitry that converts the AC audio signal into a stable DC voltage. And finally, the output voltage has a direct linear relationship to the decibel value.

Components Required:

![Image](https://github.com/user-attachments/assets/2d8bd57f-c2b6-4c5b-897f-68b3507d6362)

DFRobot SEN0232 Sound Level Meter

Arduino Nano

Adafruit SSD1306 0.96" OLED Display

Breadboard

Jumper Wires

USB Cable

Circuit Connections:

![Image](https://github.com/user-attachments/assets/701d0161-baed-4fe0-b2ba-4955ce101aea)

The wiring is dead simple; that's the beauty of the Gravity interface of this sensor. Just power the sensor with a 5V supply from the Arduino and plug the output wire into the A0 pin of the Arduino. For the OLED connection:

VCC - 5V

GND - GND

SDA - A4

SCL - A5

I am using the board that I have designed on my own; you can see all the details from here. It is an Arduino-compatible board with the same specs, but with some hardware modifications.  [I got mine from JLCPCB ](https://jlcpcb.com/?from=audrey3) . You can explore and try making one yourself; files attached. [Project Files (Google Drive)](https://drive.google.com/drive/folders/1WUuI7g_LavQaPB_SkB0YFTJq0HnHdQnA)

Arduino Code:

Here's the complete Arduino sketch that reads the sound level and displays it on the OLED with a visual bar graph.

<img width="996" height="601" alt="Image" src="https://github.com/user-attachments/assets/e16b4962-56f5-4355-aeb5-266747c9083f" />

Install these from the Arduino Library Manager:

1. Adafruit SSD1306 - For the OLED display
2. Adafruit GFX Library - Graphics primitives (installed automatically with SSD1306)
   
Go to Sketch > Include Library > Manage Libraries, search for Adafruit SSD1306, and install it. The GFX library will be installed as a dependency.

[Download the main code from this link.](https://drive.google.com/drive/folders/1ajFgN2IvcSS-4j3uNcSGjYzBxBIeKg9P)

<img width="838" height="1019" alt="Image" src="https://github.com/user-attachments/assets/0079ad0d-7664-426b-b94e-f01ac8ef4dbe" />

Upload this first and open the Serial Monitor at 115200 baud. You should see readings between 30 and 130 dBA, depending on your environment. A quiet room should read around 35-45 dBA.

How the Code Works:

Because the sensitivity and conversions are based on ADC and they work on a reference. If that reference voltage is stable the readings are accurate. Arduino's 5V rail can vary from 4.6V to 5.2V depending on the host. And when running on battery, over the time as the battery discharges there will be variations. To eliminate this issue, we used the `readVcc() function which takes the reference from the internal 1.1V bandgap of Arduino. This gives us the true supply voltage in millivolts, which we use in the dB calculation.

Raw sensor readings can be noisy. The code uses a circular buffer of 10 samples to compute a running average. This smooths out spikes while still responding quickly to changes (10 samples x 125ms = 1.25 seconds of averaging). This is the widely used noise cancellation technique used in ADCs.

Here is how the ADC conversion is done:

A perfectly linear 0.6V-to-2.6V range mapping to 30-130 dBA. No lookup tables, no polynomial curve fitting, no complex calibration. Just multiply by 50. That's it. 0.6V = 30 dBA, 2.6V = 130 dBA.

dbValue = (analogRead(A0) / 1024.0) * Vref * 50.0;

Sensitivity = 100 dB / 2.0V = 50 dB/V

Testing and Working:

I have tested the output in different scenarios, and it is working perfectly. Best to pair it directly with the Arduino with a soldering connection; a loose one may give you noise. The readings were stable and consistent. The running average smooths out transient spikes nicely, and the OLED display updates smoothly without flickering.

Testing with Songs:

![Image](https://github.com/user-attachments/assets/01175c00-599c-4927-922f-734f29e21078)

Testing during conversation:

![Image](https://github.com/user-attachments/assets/723a8f88-bc61-4d61-85c9-f4ccc5148a6e)

Conclusion:

![Image](https://github.com/user-attachments/assets/cfe18bd8-effe-4466-bed9-43e46bee2a74)

With this standalone handheld unit, we can now make instant readings, which means just power on and read, no booting or launching software.  An OLED display gives you a clear, real-time reading. Now, at least I have added one more measurement unit in my lab. I can assure you that this is the one if you are looking for a low-cost but accurate dBA measurement unit. There are a lot of other applications where this board can be used. I will come with some other applications in the future. Until then, the comment section is free for you.
