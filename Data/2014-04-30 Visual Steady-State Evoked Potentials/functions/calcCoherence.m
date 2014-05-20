function [coherence,yx_raw,yx_filt]=calcCoherence(fftx,ffty,nave)

% Basic Form: http://ocw.mit.edu/courses/earth-atmospheric-and-planetary-sciences/12-864-inference-from-data-and-models-spring-2005/lecture-notes/tsamsfmt_1_18.pdf
% Add Averaging: http://www.dsprelated.com/dspbooks/mdft/Coherence_Function.html
% Mostly worthless: http://en.wikipedia.org/wiki/Coherence_(signal_processing)


xx = fftx.*conj(fftx);
yy = ffty.*conj(ffty);
yx_raw = conj(fftx).*(ffty);
%coherence = ((abs(yx)).^2)./(xx.*yy);  %this always returns 1.0...can't be right

%ahh, need averaging to see if movement from block to block coherent
b = 1/nave*ones(nave,1);  %do a moving average filter over nave blocks
yx_filt = filter(b,1,yx_raw')';  %must filter before the ABS operation (the ABS is later)
xx = filter(b,1,xx')';
yy = filter(b,1,yy')';
coherence = (abs(yx_filt).^2)./ (xx.*yy);  