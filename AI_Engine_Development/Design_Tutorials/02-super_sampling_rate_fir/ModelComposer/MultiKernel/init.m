DISPLAY = 0;

%Frame size
N = 4;

Norm = 32768;
i = sqrt(-1);
UTaps = CreateFilter(32);
Taps = NormalizeCoef(UTaps,32768);

if(DISPLAY==1)
fprintf('---------------------------------------------------------------------\n');
fprintf('Normalization Coef %8d --> Result %d\n\n',Norm,sum(IntCoef));
fprintf('Real Part     : %8d  ->  %8d\n',min(real(IntCoef)),max(real(IntCoef)));
fprintf('Imaginary Part: %8d  ->  %8d\n',min(imag(IntCoef)),max(imag(IntCoef)));
fprintf('---------------------------------------------------------------------\n\n\n');
Lspec = 2048;
freqz(Taps/(Norm/Lspec),Lspec,'whole');
end

SaveTaps(Taps,'src/FilterTaps.txt');


SHIFT_ACC = 15;
BURST_LENGTH = 512;

Taps_4 = fliplr(Taps(1:8));
Taps_3 = fliplr(Taps(9:16));
Taps_2 = fliplr(Taps(17:24));
Taps_1 = fliplr(Taps(25:32));

% Specfying Location Constraints

LOCATION_0_0 = [25,0];

%PLIO Frequency

PLIO_FREQ = 500;
