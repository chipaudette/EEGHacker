
%given_amp_counts = 4.5/2.4e-3;

f_lim = [0 75];
pname = 'SavedData\';


fname = 'openBCI_raw_2014-08-18_15-56-50_8chanTestSig.txt'; nchan=3;
fname = 'openBCI_raw_2014-08-18_16-10-52_8chanTestSig_diedEarly.txt'; nchan=3;
%fname = 'openBCI_raw_2014-05-08_21-24-31_countbackby3_countbackby41from1200.txt';nchan=3;f_lim = [0 100];
scale_fac_volts_count=2.23e-8;




%% load data
data_uV = load([pname fname]);  %loads data as microvolts
data_uV = data_uV(:,[1:nchan+1]);  
%fs = data2.fs_Hz;
fs = 250;
count = data_uV(:,1);  %first column is a packet counter (though it's broken)
data_V = data_uV(:,2:end) * 1e-6; %other columns are data
clear data_uV;

% %% filter data
% data_V = data_V - ones(size(data_V,1),1)*mean(data_V);
% %[b,a]=butter(2,[0.2 50]/(fs/2));
% [b,a]=butter(2,0.2/(fs/2),'high');
% data_V = filter(b,a,data_V);
% [b,a]=butter(3,[55 65]/(fs/2),'stop');
% data_V = filter(b,a,data_V);
% % [b,a]=butter(3,[65 75]/(fs/2),'stop');
% % data_V = filter(b,a,data_V);

%% write to WAV
% fs_dec = fs;foo_V = data_V;
% %foo_V = resample(data_V,1,2); fs_dec = fs / 2;  %decimate
% outfname = ['WAVs\' fname(1:end-4) '.wav'];
% disp(['writing to ' outfname]);
% wavwrite(foo_V(:,1:min([size(data_V,2) 2]))*1e6/500,fs_dec,16,outfname);


%% analyze data
mean_data_V = mean(data_V);
median_data_V = median(data_V);
std_data_V = std(data_V);
spread_data_V = diff(xpercentile(data_V,0.5+(0.68-0.5)*[-1 1]))/2;
spread_data_V = median(spread_data_V)*ones(size(spread_data_V));

%% analyze dropouts
d_count =diff(count);
Iskip=find((d_count ~= 1) & (d_count ~= -255) & (d_count < 256));
n_events = length(Iskip);
n_missing = sum(d_count(Iskip))-length(Iskip);

figure;setFigureTall;
subplot(2,1,1);
plot([1:length(count)]/fs,count);
xlabel('Receive Counter');
ylabel('Transmit Counter');
ylim([0 255]);
xl=xlim;

subplot(2,1,2);
x = [2:length(count)];
plot(x/fs,d_count);
ylim([0 20]);
xlim(xl);
hold on;
plot(x(Iskip)/fs,d_count(Iskip),'rx');
xlabel(['Receive Counter']);
ylabel(['Change in Transmit Counter']);
weaText({[num2str(n_events) ' Drop Outs'];
        [num2str(n_missing) ' Packets are Missing'];
         },2);

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
    %ylim([-200 200]);
    yl=ylim;
    ylim([max([yl(1) -4000]) min([yl(2) 4000])]);
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
    N=512;overlap = 1-1/16;plots=0;
    %yl=[0 fs/2];
    [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan)*1e6,N,overlap,fs,plots);
    wT = wT + (N/2)/fs;
    
    %smooth in time
%     n_ave = 1;
%     b = 1/n_ave* ones(n_ave,1);a=1;
%     pD = filter(b,a,pD')';
    
    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    xlabel('Time (sec)');
    ylabel('Frequency (Hz)');
    ylim(f_lim);
    title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
    set(gca,'Clim',+20+[-40 0]+10*log10(256)-10*log10(N));
    %set(gca,'Clim',+140+[-90 0]+10*log10(256)-10*log10(N));
    xlim(t_sec([1 end]));
    cl=get(gca,'Clim');
    h=weaText({['Nfft = ' num2str(N) ', fs = ' num2str(fs) ' Hz'];['Clim = [' num2str(round(cl(1))) ' ' num2str(round(cl(2))) '] dB']},1);
    set(h,'BackgroundColor','white');
    ax(end+1)=gca;
end

linkaxes(ax,'x');
