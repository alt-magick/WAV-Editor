# WAV-Editor
Edit WAV Files as Spread Sheets

WAV files are made up of a series of ampliudes from quiet to loud.  
Each sample has a value for its amplitude. 
Multiple samples create sound waves that we recognize.

Convert a WAV file to CSV by typing:

wav2data.exe file.wav output.csv

Once you finish editing the CSV file in Excel or Calc,
type:

data2wav.exe output.csv new.wav

The CSV file shows the values of the right and left channels in a WAV.

As well as the time in seconds for each sample:
<br><br>

Sample, Time, Channel1, Channel2

0, 0.000000, 126, 126

1, 0.000023, 125, 125

2, 0.000045, 126, 126

3, 0.000068, 128, 128

4, 0.000091, 129, 129

5, 0.000113, 129, 129

6, 0.000136, 129, 129

7, 0.000159, 130, 130

8, 0.000181, 130, 130

9, 0.000204, 129, 129

