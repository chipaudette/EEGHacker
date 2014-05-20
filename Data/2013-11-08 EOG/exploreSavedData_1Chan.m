
%given_amp_counts = 4.5/2.4e-3;

t_excerpt_sec=[];
plottype = 'EOG';  %'ECG' or 'EEG' or 'EOG'
chan  = 1;  %adjust this for your test!
gain = 24;  %default assumed gain of OpenBCI system...overwrite as necessary
pname2 = 'SavedData\';

%fname2 = 'data_2013-11-06_18-22-20.mat'; chan = 4;gain = 24*24;  %ECG?  No tying of stomach to ground.  Kinda see T-waves.
%fname2 = 'data_2013-11-06_18-30-29.mat'; chan = 4;gain = 24*24;  %ECG?  No tying of stomach to ground.  Kinda see T-waves.
%fname2 = 'data_2013-11-07_09-30-57_ECG_wPreamp.mat'; chan = 4;gain = 24*24;  %great ECG
%fname2 = 'data_2013-11-07_09-35-25.mat'; chan = 4;gain = 24*24;   %wicked 10Hz...why?  It's not PDR Alpha, I know that.  My eyes were open.
%fname2 = 'data_2013-11-07_09-50-10.mat';chan = 2;gain = 24*24;  %failed EEG

fname2 = 'data_2013-11-08_17-25-36_EOG_V2_(NoADS627).mat';
%fname2 = 'data_2013-11-08_17-32-54_FZ_O2_EEG_V2.mat';chan=1;
%scale_fac_volts_per_count = (4.5/gain/2^24);
scale_fac_volts_per_count = (4.5/gain/2^24 * 2); %include the missing factor of two discovered ~Nov 7, 2013


%% load data
data2 = load([pname2 fname2]);  %this is scaled to "full scale" (ie +/- 1.0)

fs = data2.fs_Hz;
try
    buff_data_FS = data2.buff_data_FS;  %should be [-1.0 to +1.0]
catch
    buff_data_FS = data2.buff_data; %should be [-1.0 to +1.0]
end

%chan = 1;
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
    ylim(30*[-1 1]);
    if strcmpi(plottype,'ECG');ylim([-120 120]);end;
    if strcmpi(plottype,'EOG');ylim(600*[-1 1]);yl=ylim;end;
%     weaText({['Mean = ' num2str(mean(data2_V(:,Ichan))*1e6,3) ' uV'];
%              ['Std = ' num2str(std(data2_V(:,Ichan))*1e6,3) ' uV']},2);
    title(['OpenBCI V2,s Channel ' num2str(Ichan) ', BP = [' num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) '] Hz'],'interpreter','none');
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
