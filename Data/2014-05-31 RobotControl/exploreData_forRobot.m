
%given_amp_counts = 4.5/2.4e-3;

f_lim = [0 22];
t_lim=[];
pname = 'SavedData\';

%fname = 'openBCI_raw_2014-05-08_21-24-31_countbackby3_countbackby41from1200.txt';chans=[1:3];f_lim = [0 100];
fname = 'openBCI_raw_2014-05-31_20-57-51_RobotControl2.txt'; chans=[2];t_lim = [10 140];
scale_fac_volts_count=2.23e-8;




%% load data
data_uV = load([pname fname]);  %loads data as microvolts
data_uV = data_uV(:,[1 chans+1 size(data_uV,2)]);  %get aux, too
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
% [b,a]=butter(3,[65 75]/(fs/2),'stop');
% data_V = filter(b,a,data_V);

%% write to WAV
% fs_dec = fs;foo_V = data_V;
% %foo_V = resample(data_V,1,2); fs_dec = fs / 2;  %decimate
% outfname = ['WAVs\' fname(1:end-4) '.wav'];
% disp(['writing to ' outfname]);
% wavwrite(foo_V(:,1:min([size(data_V,2) 2]))*1e6/500,fs_dec,16,outfname);


%% analyze data
% mean_data_V = mean(data_V);
% median_data_V = median(data_V);
% std_data_V = std(data_V);
% spread_data_V = diff(xpercentile(data_V,0.5+(0.68-0.5)*[-1 1]))/2;
% spread_data_V = median(spread_data_V)*ones(size(spread_data_V));

%% plot data
t_sec = ([1:size(data_V,1)]-1)/fs;
nrow = 2; ncol=1;
ax=[];
figure;setFigureTallestWidest;
for Ichan=1:1

%     %time-domain plot
%     subplotTightBorder(nrow,ncol,(Ichan-1)*2+1);
%     plot(t_sec,data_V(:,Ichan)*1e6);
%     xlim(t_sec([1 end]));
%     %ylim(1e6*(median_data_V(Ichan)+3*[-1 1]*spread_data_V(Ichan)));
%     ylim([-200 200]);
%     weaText({['Mean = ' num2str(mean(data_V(:,Ichan))*1e6,3) ' uV'];
%         ['Std = ' num2str(std(data_V(:,Ichan))*1e6,3) ' uV']},2);
%     title(['Channel ' num2str(Ichan)]);
%     xlabel(['Time (sec)']);
%     ylabel(['Signal (uV)']);
%     ax(end+1)=gca;

    %spectrogram
    subplotTightBorder(nrow,ncol,Ichan);
    %N=1024;
    %N=1200;overlap = 1-1/32;plots=0;
    %N = 2400;overlap = 1-1/64;plots=0; yl=[0 15];
    %N=512;overlap = 1-1/16;plots=0;
     N=512;overlap = 1-50/N;plots=0;  %this is the overlap in the processing GUI
    [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan)*1e6,N,overlap,fs,plots);
    wT = wT + (N/2)/fs;
    
    %FFT Averaging (in dB space)
    if (1)
        pD_dB = 10*log10(pD);
        smooth_fac = 0.9;
        b = 1-smooth_fac; a = [1 -smooth_fac];
        pD_dB = filter(b,a,pD_dB')';  %transpose to smooth across columns
        pD = 10.^(0.1*pD_dB);
    end
    
    %continue plotting
    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    xlabel('Time (sec)');
    ylabel('Frequency (Hz)');
    title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
    set(gca,'Clim',+25+[-40 0]+10*log10(256)-10*log10(N));
    if ~isempty(t_lim)
        xlim(t_lim);
    else
        xlim(t_sec([1 end]));
    end
    xl=xlim;
    ylim(f_lim);
    cl=get(gca,'Clim');
    h=weaText({['Nfft = ' num2str(N) ', fs = ' num2str(fs) ' Hz'];['Clim = [' num2str(round(cl(1))) ' ' num2str(round(cl(2))) '] dB']},1);
    set(h,'BackgroundColor','white');
    colorbar;
    ax(end+1)=gca;
    
    %    %compute SNR
    inband_Hz = [4 15];
    Ifreq=find((f >= inband_Hz(1)) & (f <= inband_Hz(2)));
    [peak_pD,Ipeak]=max(pD(Ifreq,:));
    ave_noise_pD = zeros(size(peak_pD));
    %loop and get noise (excluding peak) for each time
    for Itime=1:length(ave_noise_pD)
        foo_pD = pD(Ifreq,Itime);
        foo_pD(Ipeak(Itime)) = NaN;
        if (Ipeak(Itime) > 1);foo_pD(Ipeak(Itime)-1) = NaN;end;
        if (Ipeak(Itime) < length(foo_pD)); foo_pD(Ipeak(Itime)+1) = NaN;end;
        ave_noise_pD(Itime) = nanmean(foo_pD);
    end
    peak_freq_Hz = f(Ifreq(Ipeak));
    snr_dB = 10*log10(pD ./ (ones(size(pD,1),1)*ave_noise_pD));
    peak_SNR_dB = zeros(size(peak_freq_Hz));
    for Itime=1:length(peak_SNR_dB);
        peak_SNR_dB(Itime) = snr_dB(Ifreq(Ipeak(Itime)),Itime);
    end
    t_snr_sec = wT;
    
    %continue plotting
    subplotTightBorder(nrow,ncol,Ichan+1);
    imagesc(wT,f,snr_dB);
    set(gca,'Ydir','normal');
    xlabel('Time (sec)');
    ylabel('Frequency (Hz)');
    title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
    set(gca,'Clim',[-10 10]);
    xlim(xl);
    ylim(f_lim);
    cl=get(gca,'Clim');
    h=weaText({['Nfft = ' num2str(N) ', fs = ' num2str(fs) ' Hz'];['Clim = [' num2str(round(cl(1))) ' ' num2str(round(cl(2))) '] dB']},1);
    set(h,'BackgroundColor','white');
    
    det_thresh_dB = 6;
    I=find(peak_SNR_dB > det_thresh_dB);
    hold on; plot(t_snr_sec(I),peak_freq_Hz(I),'wo','linewidth',2); hold off;
    
    freq_bounds = [4 6.5 9 12 15];
    for Ibound=1:length(freq_bounds);
        hold on;
        plot(xlim,freq_bounds(Ibound)*[1 1],'w:');
        hold off;
    end
    
%     hold on;
%     plot(xlim,inband_Hz(1)*[1 1],'w--','linewidth',2);
%     plot(xlim,inband_Hz(2)*[1 1],'w--','linewidth',2);
%     hold off
    colorbar;
    ax(end+1)=gca;

    
end

linkaxes(ax);
