_( side-by-side turnaround animation showing the results of two different generation parameters with the same input function )_

**Looking at the two animations above, which of the two meshes shown do you prefer aesthetically?**
(   ) Strongly left   (   ) Somewhat left   (   ) No preference   (   ) Somewhat right   (   ) Strongly right

_this question will be asked for 12 different sets of animations. No information will be provided about the technical differences between animations, to reduce potential bias_

```
_Your data will be processed in accordance with the General Data Protection Regulation 2016 (GDPR)._

_The data controller for this project will be University of Staffordshire. The university will process your personal data for the purpose of the research outlined above. The legal basis for processing your personal data for research purposes under the GDPR is a ‘task in the public interest’. You can provide your consent for the use of your personal data in this study by completing the consent form that has been provided to you._

_You have the right to access information held about you. Your right of access can be exercised in accordance with the GDPR. You also have other rights including rights of correction, erasure, objection, and data portability. Questions, comments and requests about your personal data can also be sent to the University of Staffordshire Data Protection Officer. If you wish to lodge a complaint with the Information Commissioner’s Office, please visit [www.ico.org.uk](http://www.ico.org.uk/)._
```
![[Pasted image 20251009125842.png]]


# all test configurations

a_ - asteroid
s_ - sphere
b_ - bunny
_ s - smooth
_ f - flat
b_ - BCDL
s_ - simple
_ u - unmerged
_ i - integrated
_ p - postprocessed
_ _ w - wireframe 

- [x] asteroid - gif distance=1.6, fov=60
	- [x] BCDL, unmerged
	- [x] simple, unmerged
	- [x] BCDL, integrated
	- [x] simple, integrated
	- [x] BCDL, postprocessed
	- [x] simple, postprocessed
	- [x] BCDL, integrated, wireframe
- [x] asteroid flat - as above
	- [x] BCDL, unmerged
	- [x] BCDL, integrated
	- [x] BCDL, postprocessed
- [x] sphere - defaults
	- [x] BCDL, unmerged
	- [x] simple, unmerged
	- [x] BCDL, integrated
	- [x] simple, integrated
	- [x] BCDL, postprocessed
	- [x] simple, postprocessed
	- [x] BCDL, integrated, wireframe
- [x] sphere flat - res=0.4
	- [x] BCDL, unmerged
	- [x] BCDL, integrated
	- [x] BCDL, postprocessed
	- [x] BCDL, integrated, wireframe
- [x] bunny - res=0.03, fov=50
	- [x] BCDL, unmerged
	- [x] simple, unmerged
	- [x] BCDL, integrated
	- [x] simple, integrated
	- [x] BCDL, postprocessed
	- [x] simple, postprocessed
	- [x] BCDL, integrated, wireframe

# all test pairs
bunny-BCDL-unmerged vs bunny-BCDL-integrated
bunny-BCDL-unmerged vs bunny-BCDL-postprocessed
bunny-BCDL-postprocessed vs bunny-BCDL-integrated
bunny-simple-unmerged vs bunny-simple-integrated
bunny-simple-unmerged vs bunny-simple-postprocessed
bunny-simple-postprocessed vs bunny-simple-integrated
bunny-BCDL-unmerged vs bunny-simple-unmerged
bunny-BCDL-integrated vs bunny-simple-integrated
bunny-BCDL-postprocessed vs bunny-simple-postprocessed

etc for asteroid, asteroid flat, sphere, sphere flat


order is maintained from the form design, even when shuffled!
thus all sections of the form will be laid out as above