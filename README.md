# Project
In my new kitchen I received lights that can be controlled by a remote. Here are some pictures:
![Remote Front](/Images/Remote_front.png)
![Remote Back](/Images/Remote_back.png)
![Light Back](/Images/Light_back.png)

I could not find an infrared LED on the remote, so my best guess was that it must be controlled via radio. However, the information on the labels (see pictures above) did not help me much. If an FCC ID is listed on the label you can find a lot of data online, [as explained by Michael Ossmann](https://greatscottgadgets.com/sdr/8/). But I had no such luck, neither did I find others that had worked on this specifically. So, I guess I have to do it then.

My goal is to integrate the control of the lights in my Home Assistant user interface. Preferably using an ESPHome device as a secondary remote.

# Protocol

TBA

# ESPHome

TBA

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

| On       | `01111101 00010100 01111101 00010100 [BYTE]` |
|----------|----------------------------------------------|
| Off      | `01111101 00010100 01111010 00010011 [BYTE]` |
| Brighter | `01111101 00010100 01100010 00001011 [BYTE]` |
| Dimmer   | `01111101 00010100 01100001 00001000 [BYTE]` |
| Left     | `01111101 00010100 01100000 00001001 [BYTE]` |
| Right    | `01111101 00010100 01011111 00110110 [BYTE]` |
| Button 1 | `01111101 00010100 01001111 00100110 [BYTE]` |
| Button 2 | `01111101 00010100 01001110 00100111 [BYTE]` |
| Button 3 | `01111101 00010100 01001101 00100100 [BYTE]` |
| Button 4 | `01111101 00010100 01001100 00100101 [BYTE]` |
