
%given_amp_counts = 4.5/2.4e-3;

t_excerpt_sec=[];
pname2 = 'SavedData\';
fname2 = 'data_2013-09-15_22-02-49.mat'; %noise
fname2 = 'data_2013-09-15_22-18-18.mat'; %square waves
fname2 = 'data_2013-09-15_22-28-42.mat'; %square waves
fname2 = 'data_2013-09-15_22-38-30.mat'; %noise with no AC power
fname2 = 'data_2013-09-22_14-44-17_meditator.mat';
%scale_fac_volts_per_count=2.23e-8;  %assumes ADS1299 gain is at max (which is x24?)
fname2 = 'data_2013-10-22_15-04-49.mat';
fname2 = 'data_2013-10-22_15-28-35.mat';
fname2 = 'data_2013-11-04_20-06-24_ECG_HomebrewElectrodes.mat';
fname2 = 'data_2013-11-04_20-16-06_MuWaves_HomebrewElectrodes.mat';
fname2 = 'data_2013-11-04_20-31-45_MuWaves2_HomebrewElectrodes.mat';
scale_fac_volts_per_count = (4.5/24/2^24);


%% load data
data2 = load([pname2 fname2]);  %this is scaled to "full scale" (ie +/- 1.0)

fs = data2.fs_Hz;
try
    buff_data_FS = data2.buff_data_FS;  %should be [-1.0 to +1.0]
catch
    buff_data_FS = data2.buff_data; %should be [-1.0 to +1.0]
end

chan = 1;
buff_data_FS = buff_data_FS(:,chan);

data2_counts = buff_data_FS * 2^(24-1);  %this converts back to counts...+/- 2^23 
data2_V = data2_counts * scale_fac_volts_per_count;  %apply the scale factor to get "volts"
data2_V = data2_V - mean(data2_V);

%% analyze data
mean_data2_V = mean(data2_V);
median_data2_V = median(data2_V);
std_data2_V = std(data2_V);
spread_data2_V = diff(xpercentile(data2_V,0.5+(0.68-0.5)*[-1 1]))/2;
spread_data2_V = median(spread_data2_V)*ones(size(spread_data2_V));

%% plot data
%len = min([length(data1_V) length(data2_V) length(data3_V)]);
%len = size(data2_V;
t_sec = ([1:size(data2_V,1)]-1)/fs;
%n_per_page = 3;loc1 = [1 2 3];loc2 = [4 5 6];nrow = 2; ncol=3;
nrow=2;ncol=3;%n_per_page
count=0;
for Ichan=1:size(data2_V,2);
    %count = rem(count,n_per_page);
    %if count==0;
    
        figure;setFigureTallPartWide;ax=[];
    %en
    
    count = count+1;
    subplot(2,1,1);
    %time-domain plot
    %subplot(nrow,ncol,loc1(count));
    [b,a]=butter(4,[55 65]/(fs/2),'stop');
    fdata_V = filter(b,a,data2_V(:,Ichan));
    bp_Hz = [0.5 50];
    [b,a]=butter(2,bp_Hz/(fs/2));
    fdata_V = filter(b,a,fdata_V);
    
    plot(t_sec,fdata_V*1e6);
    xlim(t_sec([1 end]));
    %ylim(1e6*(median_data2_V(Ichan)+3*[-1 1]*spread_data2_V(Ichan)));
    ylim(20*[-1 1]);
%     weaText({['Mean = ' num2str(mean(data2_V(:,Ichan))*1e6,3) ' uV'];
%              ['Std = ' num2str(std(data2_V(:,Ichan))*1e6,3) ' uV']},2);
    title([fname2 ', Channel ' num2str(Ichan) ', BP = [' num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) '] Hz'],'interpreter','none');
    xlabel(['Time (sec)']);
    ylabel(['Signal (uV)']);
        ax(end+1)=gca;

    %spectrogram
    %subplot(nrow,ncol,loc2(count));
    subplot(2,1,2);
    N=256*2;overlap = 1-1/8;plots=0;
    [pD,wT,f]=windowedFFTPlot_spectragram(data2_V(:,Ichan),N,overlap,fs,plots);
    wT = wT + (N/2)/fs;

    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    xlabel('Time (sec)');
    ylabel('Frequency (Hz)');
    %title(tt);
    %cl=get(gca,'Clim');set(gca,'Clim',cl(2)+[-60 0]);
    set(gca,'Clim',-95+[-70 0]);
    ylim([0 30]);
    xlim(t_sec([1 end]));
    cl=get(gca,'Clim');
    weaText(['Clim = [' num2str(cl(1)) ' ' num2str(cl(2)) '] dB'],1);
    ax(end+1)=gca;
    

    linkaxes(ax,'x');
end

%% plot excerpts
t_excerpt_sec = [230+[0 2]; 313.5+[0 2];202+[0 2]];
snames = {'Closed Eyes, Alpha';'Open Eyes, Relaxed, Mu';'Open Eyes, Hand Clench, No Mu'};
colors=[0 0 1;0 0.5 0; 1 0 0];
figure;setFigureTallestWidest;
%Nfft=512;foo_data=[];
for Iplot=1:size(t_excerpt_sec,1)
   
    %time domain
    subplot(2,2,Iplot);
    plot(t_sec,fdata_V*1e6,'color',colors(Iplot,:));
    xlim(t_excerpt_sec(Iplot,:));
    ylim(15*[-1 1]);
    title([snames{Iplot}],'interpreter','none');
    xlabel(['Time (sec)']);
    ylabel(['Signal (uV)']);
    xl=xlim;
    set(gca,'XTick',[xl(1):0.5:xl(2)]);
    drawnow
    weaText({['60Hz Notch'];[num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz Bandpass']},2);
    
%     inds = round(t_excerpt_sec(Iplot,1)*fs);
%     inds = inds+[1:Nfft];
%     foo_data(:,Iplot) = data2_V(inds);
end

t_excerpt_sec = [[225.5 265.5];[289.5 325.5];[190 215]];
snames = {'Closed Eyes';'Open Eyes, Relaxed';'Open Eyes, Hand Clenched'};
colors=[0 0 1;0 0.5 0; 1 0 0];

%freq domain
subplot(2,2,4);
for Iplot=1:size(t_excerpt_sec,1)
    inds = round(t_excerpt_sec(Iplot,:)*fs);
    inds = [inds(1):inds(2)];
    %[power,freqs]=fftplot(foo_data,fs,'hanning');
    N=512;overlap=0.75;
    [PSD,freq_Hz]=windowedFFTPlot(data2_V(inds),N,overlap,fs,0,'hanning');
    Hz_per_bin = fs/N;
    volts_per_sqrtHz = sqrt(PSD) / Hz_per_bin;

    semilogy(freq_Hz,volts_per_sqrtHz*1e6,'linewidth',2,'color',colors(Iplot,:));
    hold on;
end

xlabel(['Frequency (Hz)']);
ylabel(['Signal Strength (uV/sqrt(bin))']);
xlim([0 40]);
ylim([0.1 10]);set(gca,'YTick',[0.01 0.1 1 10 100],'YTickLabel',{'0.01' '0.1' '1' '10' '100'});
%squeezeAxisLeft(gca);
h=legend(snames);
%moveLegendToSide(h);
title('Comparing Spectra from C3-Cz Using Homemade Electrodes');
drawnow;
weaText({['fs = ' num2str(fs) ' Hz'];['N = ' num2str(N)]},3);
    

    
    