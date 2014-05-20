function [loop_wav,end_phase_frac] = loopWaves(wav,npoints,init_phase_frac);

if (nargin < 3)
    init_phase_frac = 0;
end

%loop the wave and put it in the new signal
new_inds = [1:npoints];
ind_into_wav = (new_inds - new_inds(1));
ind_into_wav = round(ind_into_wav + init_phase_frac*length(wav)); %make it start at the same place the last one left off
ind_into_wav = (ind_into_wav - floor(ind_into_wav/length(wav))*length(wav))+1;
loop_wav = wav(ind_into_wav);

end_phase_frac = ind_into_wav(end) / length(wav);