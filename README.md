# WAV-Editor
Edit WAV Files As Spread Sheets

WAV files are made up of a series of ampliudes from quiet to loud.  

Each sample has an amplitude represented by a number.

Multiple samples create sound waves that we recognize.

This program creates a spread sheet made up of each sample in a wav file.

Currently only 8-bit PCM WAV files are supported.

To convert a WAV file into CSV type:

wav2data.exe file.wav output.csv

You can now edit it in Excel or Calc.

And ChatGPT can modify your CSV file with AI.

When you finish type:

data2wav.exe output.csv new.wav

And you will have a new WAV file.

The CSV file shows the values of the right and left channels in a WAV.

As well as the time in seconds for each sample.
<br><br>

Your csv file should look something like this:
<br><br>

| Sample  | Time | Channel 1  | Channel 2 |
| ------------- | ------------- | ------------- | ------------- |
| 1  | 0  | 139 | 139  |
| 2  | 0.000045  | 136  | 136  |
| 3  | 0.000091  | 117  | 117  |
| 4  | 0.000136  | 120  | 120  |
| 5  | 0.000181  | 140  | 140  |
| 6  | 0.000227 | 119  | 119  |
| 7  | 0.000272 | 138  | 138  |
| 8  | 0.000317  | 116  | 116  |
| 9  | 0.000363  | 136 | 136  |
| 10  | 0.000408  | 116  | 116  |



