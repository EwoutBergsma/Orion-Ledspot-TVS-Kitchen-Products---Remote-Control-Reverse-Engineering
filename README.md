# Project
In my new kitchen I received lights that can be controlled by a remote. Here are some pictures:

|![Remote Front](/Images/Remote_front.png) Front of the remote|![Remote Back](/Images/Remote_back.png) Back of the remote|
|-------------------------------------------------------------|----------------------------------------------------------|
|TBA Front of the light                                       | ![Light Back](/Images/Light_back.png) Back of the light  |



I could not find an infrared LED on the remote, so my best guess was that it must be controlled via radio. However, the information on the labels (see pictures above) did not help me much. If an FCC ID is listed on the label you can find a lot of data online, [as explained by Michael Ossmann](https://greatscottgadgets.com/sdr/8/). But I had no such luck, neither did I find others that had worked on this specifically. So, I guess I have to do it then.

My goal is to integrate the control of the lights in my Home Assistant user interface. Preferably using an ESPHome device as a secondary remote.

# Protocol

The 433Mhz protocol sends:
- 9 ms preamble pulse, followed by 4.6ms of silence
- 5 bytes using [Pulse Position Modulation](https://triq.org/rtl_433/PULSE_FORMATS.html#ppm-%E2%80%94-pulse-position-modulation) with fixed pulse width
    - of which 4 bytes corresponding to the table below
    - last byte, in binary: `0 <3 counting bits> 1 <inverse of counting bits>`


| Button   | Message (in hex)     |
|----------|----------------------|
| On       | `7D 14 7D 14 [BYTE]` |
| Off      | `7D 14 7A 13 [BYTE]` |
| Brighter | `7D 14 62 0B [BYTE]` |
| Dimmer   | `7D 14 61 08 [BYTE]` |
| Left     | `7D 14 60 09 [BYTE]` |
| Right    | `7D 14 5F 36 [BYTE]` |
| 1        | `7D 14 4F 26 [BYTE]` |
| 2        | `7D 14 4E 27 [BYTE]` |
| 3        | `7D 14 4D 24 [BYTE]` |
| 4        | `7D 14 4C 25 [BYTE]` |

# ESPHome

An effective way of integrating it into my Home Assistant home automation is by using ESPHome. To do so I hooked up an ESP32 to a [433Mhz transmitter](https://www.hackerstore.nl/Artikel/572). Then all what was needed was to write some yaml. The complete yaml file can be found in `kitchen-light-controller.yaml`. The part that is special for this project is:

```
remote_transmitter:
  pin: 13
  carrier_duty_percent: 100%

button:
  - platform: template
    name: "Turn On"
    on_press:
      - remote_transmitter.transmit_raw:
          code: [9000, -4580, 560, -1760, 560, -608, 560, -608, 560, -608, 560, -608, 560, -608, 560, -1760, 560, -608, 560, -1760, 560, -1760, 560, -1760, 560, -608, 560, -1760, 560, -608, 560, -1760, 560, -1760, 560, -1760, 560, -608, 560, -608, 560, -608, 560, -608, 560, -608, 560, -1760, 560, -608, 560, -1760, 560, -1760, 560, -1760, 560, -608, 560, -1760, 560, -608, 560, -1760, 560, -1760, 560, -1760, 560, -608, 560, -608, 560, -1760, 560, -608, 560, -1760, 560, -1760, 560, -608, 560]
          repeat:
            times: 3
```
The `remote_transmitter` initializes the transmitter. Then using the [transmit_raw](https://esphome.io/components/remote_transmitter.html#:~:text=remote_transmitter.transmit_raw%20Action) a code can be made, where positive numbers mean a sine wave at 433Mhz is transmitted for that amount in microsecond. Likewise a negative numbers means no sine signal is transmitted for that amount in microseconds. Using the Arduino code found in `Arduino_code/Remote_control_hack_poc/Remote_control_hack_poc.ino` I could proof that, as long as anything in the communication is different, the receiver will just take it, regardless whether the end byte was changed or not. Thus, ignoring the brightness controls and the next and previous buttons, I can always send the same last byte. Because, for all other buttons I don't care that a second message would be ignored. For example, a second lights off message will be ignored (regardless of who sent those messages), which is no problem, it can't be off twice.

The original remote also repeats the message three times, so the same behaviour is implemented in our ESPHome alternative.

# The Journey/Analysis

I like to show you how I got to the above. To show you how I learned this, that this is no magic, and certainly to show you that this took effort. Hopefully this can encourage others.


## Spectrum analyzer

Coincidentally I had borrowed a spectrum analyzer for another (work) project. Of course I could not resist to use it for this project.

![Spectrum Analyzer](/Images/Spectrum%20analyzer.jpg)

Look, it is transmitting at 433Mhz! Perhaps not too much of a surpise, many such devices use it. What's just outside of the picture is my antenna, it is literally a jumper wire, I had nothing else, haha.

## RTL Software Defined Radio

That was probably not necessary, but fun nevertheless. I own one of these cheap [RTL SDRs](https://www.rtl-sdr.com/rtlsdr4everyone-review-of-5-rtl-sdr-dongles/). With custom firmware and some software on your computer you can do awesome stuff with them, for cheap. For example, using [Universal Radio Hacker](https://github.com/jopohl/urh) I was able to capture this signal:

![URH](/Images/Universal%20Radio%20Hacker.png)

At this point I was quite stuck. I spend some time trying to analyze the bits that you can see at the bottom of the figure. Quite extensively I compared this data to other data that I capture. No dice, that was not the right approach.

Whereas this looks like Amplitude Shift Keying (or On Off Keying), it is not quite that. But what is it?

## Modulation

I posted my question [on Reddit](https://www.reddit.com/r/RTLSDR/comments/1gzitvc/help_decoding_this_signal/), and one user recommended me to use the [RTL_433](https://github.com/merbanan/rtl_433) software package with the following command (on Windows): `./rtl_433.exe -A`. My terminal was exploding with data that I was receiving. To block out unwanted external signals I needed a Faraday Cage. I do not own a Faraday Cage. Luckily I have a metal wastebin. Placing my remote and RTL SDR antenna in it helped block out (most of) the unwanted signals. The software occasionally would spit out: 

`Guessing modulation: Pulse Position Modulation with fixed pulse width`

Bingo! I think.

![PPM](/Images/PPM.png)
The signal starts with a preamble (green), that's common. It is to signal that a message is about to come, plus it can be used for synchronization. After it we see a series of pulses that all have equal width. Between these pulses there are long periods (red) and short periods (blue) of nothing. By eyeballing we can see that, at least roughly, all these pauses are either equaly long as the red one or the blue one (ignoring the pause after the preamble). Neat!

Also, what I have not told you yet, every button press makes the remote controll send three such identical messages. Not too important for now.

## Let's demodulate!

By considering the long pauses as 0s and the short pauses as 1s, I demodulated me pressing the same button over and over:
- 10000010 11101011 10110011 11011010 1 001 0 110 
- 10000010 11101011 10110011 11011010 1 000 0 111
- 10000010 11101011 10110011 11011010 1 110 0 001
- 10000010 11101011 10110011 11011010 1 101 0 010
- 10000010 11101011 10110011 11011010 1 100 0 011
- 10000010 11101011 10110011 11011010 1 011 0 100
- 10000010 11101011 10110011 11011010 1 010 0 101
- 10000010 11101011 10110011 11011010 1 001 0 110
- 10000010 11101011 10110011 11011010 1 000 0 111
- 10000010 11101011 10110011 11011010 1 110 0 001

You may notice, that's all very similar. Except for the last byte (8-bits). For convenience I have split this up. The last three bits appear to be counting in binary. Neat, that's easy. The bit before it appears to be always 0, sure. After some puzzling I realised that the three bits before that are always the inverse of the three counting bits. 

Hold on. Perhaps long pauses represent 1s and short pauses represent 0s. Because, then all bits should be inverted. Then the three inverted bits are the counting bits and what I previously thought were the counting bits are a checksum. That's probably it.

Let's do all buttons now:

| Button   | Message                                      |
|----------|----------------------------------------------|
| On       | `01111101 00010100 01111101 00010100 [BYTE]` |
| Off      | `01111101 00010100 01111010 00010011 [BYTE]` |
| Brighter | `01111101 00010100 01100010 00001011 [BYTE]` |
| Dimmer   | `01111101 00010100 01100001 00001000 [BYTE]` |
| Left     | `01111101 00010100 01100000 00001001 [BYTE]` |
| Right    | `01111101 00010100 01011111 00110110 [BYTE]` |
| 1        | `01111101 00010100 01001111 00100110 [BYTE]` |
| 2        | `01111101 00010100 01001110 00100111 [BYTE]` |
| 3        | `01111101 00010100 01001101 00100100 [BYTE]` |
| 4        | `01111101 00010100 01001100 00100101 [BYTE]` |
