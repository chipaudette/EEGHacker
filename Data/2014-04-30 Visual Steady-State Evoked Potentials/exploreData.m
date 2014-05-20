
%given_amp_counts = 4.5/2.4e-3;


pname = 'SavedData\';
fname = 'openBCI_raw_2014-04-05_16-25-11_countBackAfterLastAlpha_filt.txt';nchan=1;
%fname = 'openBCI_raw_2014-04-05_16-31-37_ECGelec_impedanceChecks_filt.txt';nchan=2;
%fname = 'openBCI_raw_2014-04-05_16-36-34_ECGelec_countBackBy3_noFilt.txt';nchan=2;
%fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=2;
fname = '10-openBCI_raw_2014-04-19_10-23-36_EyesClosed_8secBreaths.txt';nchan=2;
 fname = '11-openBCI_raw_2014-04-19_10-32-36_StartedBackBy3s.txt';nchan=2;
% fname = '12-openBCI_raw_2014-04-19_10-40-38_countbackwardsby3.txt';nchan=2;
% fname = '13-openBCI_raw_2014-04-19_10-54-51_bothOnForehead_countback.txt';nchan=2;
fname = 'openBCI_raw_2014-04-23_06-52-48_Breakfast_Birds_CountBack.txt';nchan=1;
fname = 'openBCI_raw_2014-04-30_22-14-48_fullRun_4blocks.txt';nchan=2;

scale_fac_volts_count=2.23e-8;




%% load data
data_uV = load([pname fname]);  %loads data as microvolts
data_uV = data_uV(:,[1:nchan+1]);  
%fs = data2.fs_Hz;
fs = 250;
count = data_uV(:,1);  %first column is a packet counter (though it's broken)
data_V = data_uV(:,2:end) * 1e-6; %other columns are data
clear data_uV;

%% filter data
data_V = data_V - ones(size(data_V,1),1)*mean(data_V);
%[b,a]=butter(2,[0.2 50]/(fs/2));
[b,a]=butter(2,0.2/(fs/2),'high');
data_V = filter(b,a,data_V);
[b,a]=butter(3,[55 65]/(fs/2),'stop');
data_V = filter(b,a,data_V);
[b,a]=butter(3,[65 75]/(fs/2),'stop');
data_V = filter(b,a,data_V);

%% write to WAV
fs_dec = fs;foo_V = data_V;
%foo_V = resample(data_V,1,2); fs_dec = fs / 2;  %decimate
outfname = ['WAVs\' fname(1:end-4) '.wav'];
disp(['writing to ' outfname]);
wavwrite(foo_V(:,1:min([size(data_V,2) 2]))*1e6/500,fs_dec,16,outfname);


%% analyze data
mean_data_V = mean(data_V);
median_data_V = median(data_V);
std_data_V = std(data_V);
spread_data_V = diff(xpercentile(data_V,0.5+(0.68-0.5)*[-1 1]))/2;
spread_data_V = median(spread_data_V)*ones(size(spread_data_V));

%% plot data
t_sec = ([1:size(data_V,1)]-1)/fs;
nrow = max([2 size(data_V,2)]); ncol=2;
ax=[];
figure;setFigureTallestWidest;
for Ichan=1:size(data_V,2);

    %time-domain plot
    subplotTightBorder(nrow,ncol,(Ichan-1)*2+1);
    plot(t_sec,data_V(:,Ichan)*1e6);
    xlim(t_sec([1 end]));
    %ylim(1e6*(median_data_V(Ichan)+3*[-1 1]*spread_data_V(Ichan)));
    ylim([-200 200]);
    weaText({['Mean = ' num2str(mean(data_V(:,Ichan))*1e6,3) ' uV'];
        ['Std = ' num2str(std(data_V(:,Ichan))*1e6,3) ' uV']},2);
    title(['Channel ' num2str(Ichan)]);
    xlabel(['Time (sec)']);
    ylabel(['Signal (uV)']);
    ax(end+1)=gca;

    %spectrogram
    subplotTightBorder(nrow,ncol,(Ichan-1)*2+2);
    %N=1024;
    %N=1200;overlap = 1-1/32;plots=0;
    %N = 2400;overlap = 1-1/64;plots=0; yl=[0 15];
    N=500;overlap = 1-1/16;plots=0;
    %yl=[0 fs/2];
    yl = [0 25];
    [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan),N,overlap,fs,plots);
    wT = wT + (N/2)/fs;
    
    %smooth in time
%     n_ave = 1;
%     b = 1/n_ave* ones(n_ave,1);a=1;
%     pD = filter(b,a,pD')';
    
    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    if (Ichan > 6); xlabel('Time (sec)');end
    ylabel('Frequency (Hz)');
    title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
    %cl=get(gca,'Clim');set(gca,'Clim',cl(2)+[-60 0]);
    set(gca,'Clim',-86+[-50 0]+10*log10(256)-10*log10(N));
    ylim(yl);
    xlim(t_sec([1 end]));
    %cl=get(gca,'Clim');
    %weaText(['Clim = [' num2str(cl(1)) ' ' num2str(cl(2)) '] dB'],1);
    ax(end+1)=gca;
end

linkaxes(ax,'x');
