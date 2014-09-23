---
layout: post
comments: true
title: LCD Photo Frame
categories: [projects]
tags: [projects, woodworking, electronics]
---
<div class="row">
		<a href="/images/oldprojects/lrpfparts.jpg" class="fancybox" rel="clock"><img src="/images/oldprojects/lrpfparts-150x112.jpg" alt="Parts for LCD photo frame."></a>
		<a href="/images/oldprojects/lrphotoframe.jpg" class="fancybox" rel="clock"><img src="/images/oldprojects/lrphotoframe-150x112.jpg" alt="Finished LCD photo frame."></a>
</div>

### Update

I'm happy to report that as of 2014-09-23 this photo frame machine still works. It was built over 10 years ago, in a time before you could buy the equivalent off the shelf at Walmart.

### The Idea

Before LCD photo frames were widely available, we decided to build one for my Dad for a Father's Day gift. We had collected thousands of pictures in our online photo gallery, but had no easy way to display them.

At the time, I saw an article in Popular Science describing a project that took laptop and made it into an LCD photo frame. I realized that I could so the same or better utilizing a mini-computer, and an lcd screen.

There are several more pictures of this project are available in the [Photo Gallery](http://gallery.chuckhays.net/Projects/LCDPhotoFrame/).

### The Design

I decided to use a VIA EPIA mini-itx motherboard. Along with this I purchased a 12V power supply, a 12V adapter, and a laptop hard drive for storage. At the time, large and inexpensive flash memory was not available. If I were to do it again, I would add a card reader like the commercial products have. All of these parts would be mounted inside a wooden frame on the back of the LCD monitor.

### Building

The wooden frame was built, and the LCD screen was mounted inside, after removing the built in stand. The various computer parts were assembled, and were placed inside the frame, and attached with velcro to the back of the LCD monitor. On the side panel of the frame was mounted a panel which included the power switch, power plug, ethernet port, and usb plugs. This allows the frame to be hooked up to a network, and connected to via the network. Alternatively, a keyboard may be attached to the usb port.

### Software

I initially booted Knoppix to see if linux would have the needed modules to run on the VIA EPIA board/cpu, and it worked flawlessly. Fortunately it also had all the software I needed to build the automatic slideshow, so I simply did a hard drive install of knoppix, and set it to automatically run the slideshow software. Pictures are added by uploading them to gallery software (older version of the software running our photo gallery).
