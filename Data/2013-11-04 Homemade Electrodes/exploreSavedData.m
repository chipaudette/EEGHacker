
%given_amp_counts = 4.5/2.4e-3;


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
scale_fac_volts_per_count = (4.5/24/2^24);


%% load data
data2 = load([pname2 fname2]);  %this is scaled to "full scale" (ie +/- 1.0)
fs = data2.fs_Hz;
try
    buff_data_FS = data2.buff_data_FS;  %should be [-1.0 to +1.0]
catch
    buff_data_FS = data2.buff_data; %should be [-1.0 to +1.0]
end
data2_counts = buff_data_FS * 2^(24-1);  %this converts back to counts...+/- 2^23 
data2_V = data2_counts * scale_fac_volts_per_count;  %apply the scale factor to get "volts"

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
n_per_page = 3;loc1 = [1 2 3];loc2 = [4 5 6];nrow = 2; ncol=3;
count=0;
for Ichan=1:size(data2_V,2);
    count = rem(count,n_per_page);
    if count==0;
        figure;setFigureTallestWidest;
    end
    count = count+1;
    
    %time-domain plot
    subplot(nrow,ncol,loc1(count));
    plot(t_sec,data2_V(:,Ichan)*1e6);
    xlim(t_sec([1 end]));
    ylim(1e6*(median_data2_V(Ichan)+3*[-1 1]*spread_data2_V(Ichan)));
    weaText({['Mean = ' num2str(mean(data2_V(:,Ichan))*1e6,3) ' uV'];
             ['Std = ' num2str(std(data2_V(:,Ichan))*1e6,3) ' uV']},2);
    title(['Channel ' num2str(Ichan)]);
    xlabel(['Time (sec)']);
    ylabel(['Signal (uV)']);
    
    %spectrogram
    subplot(nrow,ncol,loc2(count));
    N=256;overlap = 1-1/16;plots=0;
    [pD,wT,f]=windowedFFTPlot_spectragram(data2_V(:,Ichan),N,overlap,fs,plots);
    wT = wT + (N/2)/fs;

    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    xlabel('Time (sec)');
    ylabel('Frequency (Hz)');
    %title(tt);
    %cl=get(gca,'Clim');set(gca,'Clim',cl(2)+[-60 0]);
    set(gca,'Clim',-80+[-80 0]);
    ylim([0 65]);
    xlim(t_sec([1 end]));
    cl=get(gca,'Clim');
    weaText(['Clim = [' num2str(cl(1)) ' ' num2str(cl(2)) '] dB'],1);

end


return

all_data = [data1_V(1:len) data2_V(1:len) data3_V(1:len)];
bp_Hz = [3 40];
[b,a]=weaFIR(fs,bp_Hz/(fs/2));
f_all_data = filter(b,a,all_data);

I=find((t_sec>t_zoom_sec(1)) & (t_sec < t_zoom_sec(2)));
t_sec = t_sec(I);
all_data = all_data(I,:);
f_all_data = f_all_data(I,:);


subplot(3,3,2*3+2);
%plot(t_sec,(all_data-ones(size(all_data,1),1)*median(all_data))*1e6,'linewidth',2);
plot(t_sec,f_all_data*1e6,'linewidth',2);
xlabel(['Time (sec)']);
ylabel(['Value (uvolts, ' num2str(bp_Hz(1)) ' - ' num2str(bp_Hz(2)) 'Hz)']);
xlim(t_zoom_sec);
%ylim([-250 250]);
legend('TI Dev Kit','Creare','Olimex');
title(sname);
weaText(['Creare = ' num2str(scale_fac_volts_per_count,3) ' V/count'],4);

subplot(3,3,2*3+3);
%[pD,freqs,N,h]=fftplot(all_data(1:N,:),fs,'hanning');
all_pD = [];
for I=1:size(all_data,2)
    foo = all_data(:,I);
    [foo,wT,freq_Hz] = windowedFFTPlot_spectragram(foo-mean(foo),N,overlap,fs,0);
    all_pD(:,I) = mean(foo')';
end
Hz_per_bin = fs / N;
pD_per_Hz = all_pD / Hz_per_bin;
semilogy(freq_Hz,sqrt(pD_per_Hz)*1e6,'linewidth',2);
ylabel(['uVolts / sqrt(Hz)']);
xlabel(['Frequency (Hz)']);
xlim([0 65]);
ylim([0.01 100]);
legend('TI Dev Kit','Creare','Olimex',4);
title(sname);

