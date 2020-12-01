
% CIC testing
% N4_R4_M2
clear all
Fs = 7812;
t= 0:1/Fs:5.0 ;
y = randn(size(t)) ;

%CIC lowpass and decimate
int1 = y(1) ;
int2 = int1 ;
int3 = int1 ;
int4 = int1 ;
j = 1 ;
nIF = 4 ;
last_c1 = 0;
last_c2 = 0;
last_c3 = 0;
last_int4 = 0;
last2_c1 = 0;
last2_c2 = 0;
last2_c3 = 0;
last2_int4 = 0;
comb4 = zeros(1,length(t));

for i=1:length(y)
  int1 = int1 + y(i) ;
  int2 = int2 + int1 ;
  int3 = int3 + int2 ;
  int4 = int4 + int3 ;
  
  if (mod(i,nIF)==0)
    comb1 = int4 - last2_int4 ;
    comb2 = comb1 - last2_c1 ;
    comb3 = comb2 - last2_c2 ;
    comb4(j) = comb3 - last2_c3 ;
    
    last2_int4 = last_int4 ;
    last_int4 = int4 ;
    last2_c1 = last_c1 ;
    last_c1 = comb1;
    last2_c2 = last_c2 ;
    last_c2 = comb2;
    last2_c3 = last_c3 ;
    last_c3 = comb3;
    
    j = j + 1 ;
  end
end

% now plot the spectrum 
figure(2);clf
pwelch(comb4,[256],[],[],Fs/nIF);

% second order compensator to flatten CIC output
figure(1);clf;
[b,a] = cheby1(2, 8, 0.5);
out = filter(b,a,comb4) ;
pwelch(out,[256],[],[],Fs/nIF);;
