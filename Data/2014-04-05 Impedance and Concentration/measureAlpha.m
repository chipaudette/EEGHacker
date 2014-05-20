
%given_amp_counts = 4.5/2.4e-3;


pname = 'SavedData\';
switch 2
%     case 1
%         %ECG Electrodes
%         fname = 'openBCI_raw_2014-04-05_16-31-37_ECGelec_impedanceChecks_filt.txt';nchan=1;
%         %t_lim_sec = [39 41.5; 46 48; 79 81;92 100; 138 144; 159 164; 178 184; 193 195;250 255 ; 263 267];
%         t_lim_sec = [46 48; 55 58; 79 81;92 100; 159 164; 178 184; 193 195;250 255];
    case 2
        fname = 'openBCI_raw_2014-04-05_16-30-54.txt';nchan=1;
        t_lim_sec = [12 21.25];
    case 3
        %ECG Electrodes
        fname = 'openBCI_raw_2014-04-05_16-36-34_ECGelec_countBackBy3_noFilt.txt';nchan=1;
        t_lim_sec = [33.5 50;86.2 91.9];
    case 4
        fname = 'openBCI_raw_2014-04-05_16-39-58.txt'; nchan=1;
        t_lim_sec = [59.9 75; 131.7 138.2];
    case 5
        %EEG Electrodes
        fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=1;
        t_lim_sec = [192.3 218.4; 261.1 267.3];
end
%fname = 'openBCI_raw_2014-04-05_16-36-34_ECGelec_countBackBy3_noFilt.txt';nchan=2;
%fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=2;
scale_fac_volts_count=2.23e-8;

current_A = 6e-9;


%% load data
data_uV = load([pname fname]);  %loads data as microvolts
data_uV = data_uV(:,[1:nchan+1]);
%fs = data2.fs_Hz;
fs = 250;
count = data_uV(:,1);  %first column is a packet counter (though it's broken)
data_V = data_uV(:,2:end) * 1e-6; %other columns are data
clear data_uV;

%% filter data around the test tone
data_V = data_V - ones(size(data_V,1),1)*mean(data_V);
[b,a]=butter(2,[55 65]/(fs/2),'stop');
data_V = filter(b,a,data_V);  %apply notch filter

bp_basic_Hz = [1 50];
[b,a]=butter(2,bp_basic_Hz/(fs/2));
bp_data_V = filter(b,a,data_V);

bp_Hz = 10+[-3 3];
%bp_Hz = [7 15];
%bp_Hz = [3 30];
Nfir = 2*round(0.5*fs);  %ensure an even number
[b,a]=weaFIR(Nfir,(bp_Hz)/(fs/2));
fdata_V = filter(b,a,data_V);
fdata_V = [fdata_V(Nfir/2+1:end,:);zeros(Nfir/2,size(fdata_V,2))];  %remove latency

%get frequency of alpha from hilbert
hfdata_V = hilbert(fdata_V);
freq_Hz = [0;unwrap(diff(angle(hfdata_V(:,1))))*fs]/(2*pi);

%filter the hilbert measurement of alpha
f_freq_Hz = freq_Hz;
lp_freq_Hz = 3;
[b,a]=butter(2,lp_freq_Hz/(fs/2));
f_freq_Hz = filter(b,a,freq_Hz);
Gd = grpdelay(b,a,lp_freq_Hz/2*[1 1],fs);
Gd = round(Gd(1));
f_freq_Hz = [f_freq_Hz(Gd+1:end,:);zeros(Gd,size(f_freq_Hz,2))]; %remove latency

%measure frequency of alpha from zero crossings
I=find((fdata_V(1:end-1) <= 0) & (fdata_V(2:end) > 0));
t_freq2_sec = 0.5*(I(1:end-1)+I(2:end))/fs;
freq2_Hz = 1./(diff(I)/fs);

%get amplitude of alpha
Nave = 2*round(0.5*fs/2); %make even
b = 1/Nave*ones(Nave,1);
a = 1;
rms_V = sqrt(filter(b,a,fdata_V.^2));
rms_V = [rms_V(Nave/2+1:end,:);zeros(Nave/2,size(rms_V,2))];  %remove filter latency

% %% construct synthetic version
% phase_rad = cumsum(freq_Hz/fs)*2*pi;
% synth_data_V = rms_V.*sqrt(2).*sin(phase_rad);
% 
% %% make high frequency version
% up_fac = 32;
% fs_new= up_fac*fs;
% freq_multiplier = 50;
% up_freq_Hz = freq_multiplier*resample(f_freq_Hz,up_fac,1);
% up_rms_V = resample(rms_V,up_fac,1);
% phase2_rad = cumsum(up_freq_Hz/fs_new)*2*pi;
% synth2_data_V = up_rms_V.*sqrt(2).*sin(phase2_rad);
% wavwrite((synth2_data_V*1e6)./200,fs_new,16,...
%     [num2str(freq_multiplier) 'x_filtFreq_' fname(1:end-3) '.wav']);

%% quantify data
mean_data_V = [];
std_data_V = [];
for Idata=1:size(t_lim_sec);
    inds = round(t_lim_sec(Idata,:)*fs);
    inds = [max([1 inds(1)]) min([size(fdata_V,1) inds(2)])];
    inds = [inds(1):inds(2)];
    mean_data_V(Idata,:) = mean(fdata_V(inds,:));
    std_data_V(Idata,:) = std(fdata_V(inds,:));
    imp_data_Ohm(Idata,:) =  std_data_V(Idata,:)*sqrt(2) / current_A;
end


%% simple plot
Ichan=1;

t_sec = ([1:size(data_V,1)]-1)/fs;

figure;setFigureTallestPartWide;ax=[];
subplotTightBorder(4,1,1);
plot(t_sec,bp_data_V(:,Ichan)*1e6);
hold on;plot(t_sec,fdata_V*1e6,'r','linewidth',2);hold off;
%hold on;plot(t_sec,synth_data_V*1e6,'g','linewidth',2);hold off;
hold on;plot(xlim,[0 0],'k--','linewidth',2);hold off;
ylabel(['EEG Data (uV)']);
ylim(40*[-1 1]);
xlim(t_sec([1 end]));xl=xlim;
%legend('Raw','Filt','Synth');
ax(end+1)=gca;



subplotTightBorder(4,1,2);
N=512;overlap = 1-1/16;plots=0;
%Ichan=1;
[pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan),N,overlap,fs,plots);
wT = wT + (N/2)/fs;
imagesc(wT,f,10*log10(pD));
set(gca,'Ydir','normal');
%if (Idata > 6); xlabel('Time (sec)');end
ylabel('Frequency (Hz)');
title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
set(gca,'Clim',-80+[-50 0]+10*log10(256)-10*log10(N));
ylim([0 20]);
xlim(t_sec([1 end]));xl=xlim;
ax(end+1)=gca;

hold on;
xl=xlim;
plot(xl,bp_Hz(1)*[1 1],'w--','linewidth',2);
plot(xl,bp_Hz(2)*[1 1],'w--','linewidth',2);
hold off

subplotTightBorder(4,1,3);
plot(t_sec,rms_V*1e6,'linewidth',2);
ylim([0 20]);
ylabel(['RMS (uV) over ' num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz']);
%xlabel(['Time (sec)']);
xlim(xl);
ax(end+1)=gca;

subplotTightBorder(4,1,4);
plot(t_sec,freq_Hz,'linewidth',2);
hold on; plot(t_sec,f_freq_Hz,'r','linewidth',2);hold off;
hold on; plot(t_freq2_sec,freq2_Hz,'.-','color',[0 1 0],'linewidth',2); hold off;
legend('Hilbert','Filt Hilbert','Zero Cross');
ylim([5 15]);
ylabel(['Frequency (Hz)']);
xlabel(['Time (sec)']);
xlim(xl);
ax(end+1)=gca;

ylim(bp_Hz+[-2 2]);

hold on;
xl=xlim;
plot(xl,bp_Hz(1)*[1 1],'k--','linewidth',2);
plot(xl,bp_Hz(2)*[1 1],'k--','linewidth',2);
hold off


for Iplot=1:4
    subplotTightBorder(4,1,Iplot);
    for Idata=1:size(t_lim_sec,1);
        hold on;
        hold on;yl=ylim;plot(t_lim_sec(Idata,1)*[1 1],yl,'g--','linewidth',2);plot(t_lim_sec(Idata,2)*[1 1],yl,'r--','linewidth',2);hold off
        hold off
        
        yl=ylim;
        if (Iplot==3)
            h=text(mean(t_lim_sec(Idata,:)),3*yl(1),{num2str(std_data_V(Idata,1)*1e6,3); 'uVrms'});
            set(h,'HorizontalAlignment','center','VerticalAlignment','bottom','backgroundcolor','white');
        end
    end
end

linkaxes(ax,'x');

return






%% plot data
t_sec = ([1:size(data_V,1)]-1)/fs;
%nrow = max([2 size(data_V,2)])+1; ncol=2;
ncol=2;
nrow = 1+ceil(size(t_lim_sec,1)/ncol);
ax=[];
figure;setFigureTallestWidest;
subplotTightBorder(nrow,1,1);
N=512;overlap = 1-1/8;plots=0;
Ichan=1;
[pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan),N,overlap,fs,plots);
wT = wT + (N/2)/fs;


imagesc(wT,f,10*log10(pD));
set(gca,'Ydir','normal');
if (Idata > 6); xlabel('Time (sec)');end
ylabel('Frequency (Hz)');
title([fname ', Channel ' num2str(Idata)],'interpreter','none');
set(gca,'Clim',-80+[-50 0]+10*log10(256)-10*log10(N));
ylim([0 40]);
xlim(t_sec([1 end]));

for Idata=1:size(t_lim_sec,1);
    hold on;
    hold on;yl=ylim;plot(t_lim_sec(Idata,1)*[1 1],yl,'g--','linewidth',2);plot(t_lim_sec(Idata,2)*[1 1],yl,'r--','linewidth',2);hold off
    hold off
end

for Idata = 1:size(t_lim_sec,1);

    %time-domain plot
    subplotTightBorder(nrow,ncol,2+Idata);
    plot(t_sec,fdata_V(:,Ichan)*1e6);
    xlim(t_sec([1 end]));
    %ylim(1e6*(median_data_V(Idata)+3*[-1 1]*spread_data_V(Idata)));
    ylim(1000*[-1 1]);

    xlim(t_lim_sec(Idata,:)+[-10 10]);xl=xlim;
    hold on;yl=ylim;plot(t_lim_sec(Idata,1)*[1 1],yl,'g--','linewidth',2);plot(t_lim_sec(Idata,2)*[1 1],yl,'r--','linewidth',2);hold off

    h=weaText({['BP = [' num2str(bp_Hz(1)) ' ' num2str(bp_Hz(2)) '] Hz'];
        ['RMS = ' num2str(std_data_V(Idata,Ichan)*1e6,3) ' uV'];
        ['Imp = ' num2str(std_data_V(Idata,Ichan)/6e-9/1000,3) ' kOhm']},2);
    set(h,'backgroundcolor','white');
    title(['Channel ' num2str(Ichan)]);
    if (Idata>size(t_lim_sec,1)-2);xlabel(['Time (sec)']);end
    ylabel(['Signal (uV)']);
    ax(end+1)=gca;

% 
%     %spectrogram
%     subplotTightBorder(nrow,ncol,(Idata)*2+2);
%     %N=1024;
%     N=1200;overlap = 1-1/32;plots=0;
%     [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Idata),N,overlap,fs,plots);
%     wT = wT + (N/2)/fs;
% 
%     imagesc(wT,f,10*log10(pD));
%     set(gca,'Ydir','normal');
%     if (Idata > 6); xlabel('Time (sec)');end
%     ylabel('Frequency (Hz)');
%     title([fname ', Channel ' num2str(Idata)],'interpreter','none');
%     set(gca,'Clim',-80+[-50 0]+10*log10(256)-10*log10(N));
%     ylim([0 40]);
%     xlim(t_sec([1 end]));
%     ax(end+1)=gca;
% 
%     xlim(xl);
%     hold on;yl=ylim;plot(t_lim_sec(Idata,1)*[1 1],yl,'w--','linewidth',2);plot(t_lim_sec(Idata,2)*[1 1],yl,'w--','linewidth',2);hold off
end

%linkaxes(ax);
