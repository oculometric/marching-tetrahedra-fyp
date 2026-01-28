@setlocal ENABLEDELAYEDEXPANSION
@echo off
echo let's get to work!

set first[0]=bu
set secnd[0]=bi
set first[1]=bu
set secnd[1]=bp
set first[2]=bp
set secnd[2]=bi
set first[3]=su
set secnd[3]=si
set first[4]=su
set secnd[4]=sp
set first[5]=sp
set secnd[5]=si
set first[6]=bu
set secnd[6]=su
set first[7]=bi
set secnd[7]=si
set first[8]=bp
set secnd[8]=sp

for %%p in (ss,sf,bs,as,af) do (
	set prefix=%%p
	for %%n in (0,1,2,3,4,5,6,7,8) do (
		set "input_a=!prefix!_!first[%%n]!"
		set "input_b=!prefix!_!secnd[%%n]!"
		set "output=!input_a!_!input_b!"
		.\bin\ffmpeg.exe -i .\renders\!input_a!.gif -i .\renders\!input_b!.gif -filter_complex "hstack,scale=-1:1024:flags=neighbor" -c:v libx264 -preset slow -crf 12 -profile:v high -pix_fmt yuv420p !output!.mp4
	)
	pause
)