

addpath('functions\');

%pname = 'SaveData_ProcessingGUI\';
% fname = 'openBCI_raw_2013-11-11_13-55-54.txt'; %dummy data...8-channels
% fname = 'openBCI_raw_2013-11-14_16-13-26.txt';  %dummy data...16-channels
%fname = 'openBCI_raw_2013-12-24_13-53-54_rxc_relaxation.txt'; t_closed_sec = [125 350];sname='Relaxing';
%fname = 'openBCI_raw_2013-12-24_13-26-11_rxc_meditation.txt'; t_closed_sec = [350 1050];sname='Meditating';
%scale_fac_volts_count=2.23e-8;

Nfft = 256; f_lim = [0 100];
pname = 'SavedData\';
t_mark_sec = [];mark_names={};
t_plot_sec=[];
chan_names={};
switch 20
    case 10
        %here, elec1 was on left forehead and elec2 was on back of head
        %ref on left ear lob
        pname = '..\2014-04-05 Impedance and Concentration\SavedData\';
        fname = '10-openBCI_raw_2014-04-19_10-23-36_EyesClosed_8secBreaths.txt';
        if (1)
            t_sig_sec = [285 336];
            t_baseline_sec = [187 213];
        else
            t_sig_sec = [215 285];
            t_baseline_sec = [345 400];
        end
        t_mark_sec = [t_sig_sec;t_baseline_sec];
    case 12
        %here, elec1 was on left forehead and elec2 was on back of head
        %ref on left ear lob
        pname = '..\2014-04-05 Impedance and Concentration\SavedData\';
        fname = '12-openBCI_raw_2014-04-19_10-40-38_countbackwardsby3.txt';
        t_sig_sec = [90 130];
        %t_baseline_sec = [40 55];
        t_baseline_sec = [155 179];
        t_mark_sec = [40 55; t_sig_sec;t_baseline_sec];
        mark_names = {'Eyes Closed';'Count Back by 3';'Eyes Closed'};
    case 13
        %here, elec1 was on left forehead and elec2 was on right forehead.
        %ref on left ear lobe
        pname = '..\2014-04-05 Impedance and Concentration\SavedData\';
        fname = '13-openBCI_raw_2014-04-19_10-54-51_bothOnForehead_countback.txt';
        %t_sig_sec = [63 84];
        t_sig_sec = [100 113];
        %t_baseline_sec = [47 55];
        t_baseline_sec = [65 80];
        t_mark_sec = [47 55; t_sig_sec;t_baseline_sec];
        mark_names = {'Eyes Closed';'Count Back by 3';'Eyes Closed'};
        t_analyze_sec = t_mark_sec;
    case 20
        %here, elec1 was on left forehead and elec2 was on right forehead.
        %ref on left ear lobe
        fname = 'openBCI_raw_2014-04-23_06-52-48_Breakfast_Birds_CountBack.mat';
        sname = '2014-04-23 Breakfast, Web, Birds, Concentration';
        chan_names = {'Left Forehead';'Right Forehead'};
        t_mark_sec = [17*60+43 21*60+31 26*60+41-5  29*60+23 31*60+2];
        t_mark_sec = [t_mark_sec(1:end-1)' t_mark_sec(2:end)'];
        mark_names = {'Gaze Outside','Internet','Eyes Closed','Count Back by 3'};
        if 1
            t_mark_sec = [719 967;t_mark_sec];
            mark_names = {'Internet',mark_names};
        end
        t_plot_sec = [680 1890];
end
t_analyze_sec = t_mark_sec;
N = Nfft;
t_lim_sec = [];
if ~isempty(t_mark_sec)
    t_lim_sec = [t_mark_sec(1) t_mark_sec(end)];
end


compare_chan = [2 1;
    ];


%% load data
data_uV = load([pname fname]);  %loads data as microvolts
if isstruct(data_uV);data_uV = data_uV.data_uV;end;
%fs = data2.fs_Hz;
fs = 250;
count = data_uV(:,1);  %first column is a packet counter (though it's broken)
data_V = data_uV(:,2:end) * 1e-6; %other columns are data
clear data_uV;

data_V = data_V(:,1:2);  %keep just these channels

%% filter data
data_V = data_V - ones(size(data_V,1),1)*mean(data_V);
[b,a]=butter(2,[1 100]/(fs/2));
data_V = filter(b,a,data_V);


%% analyze data
mean_data_V = mean(data_V);
median_data_V = median(data_V);
std_data_V = std(data_V);
spread_data_V = diff(xpercentile(data_V,0.5+(0.68-0.5)*[-1 1]))/2;
spread_data_V = median(spread_data_V)*ones(size(spread_data_V));

%% plot data ...time domain
%len = min([length(data1_V) length(data_V) length(data3_V)]);
%len = size(data_V;
t_sec = ([1:size(data_V,1)]-1)/fs;
n_per_page = 8;%loc1 = [1 2 3];loc2 = [4 5 6];
nrow = 4; ncol=2;
count=0;
ax=[];
mean_pD=[];


%% plot data ...spectrogram
%len = min([length(data1_V) length(data_V) length(data3_V)]);
%len = size(data_V;
t_sec = ([1:size(data_V,1)]-1)/fs;
if isempty(t_plot_sec); t_plot_sec = t_sec([1 end]);end;
n_per_page = 8;%loc1 = [1 2 3];loc2 = [4 5 6];
%nrow = 4; ncol=2;
nrow = 3;ncol=1;
count=0;
ax=[];
mean_pD=[];
for Ichan=1:size(data_V,2);
    count = rem(count,n_per_page);
    if count==0;
        %figure;setFigureTallestWidest;
        figure;setFigureTallestPartWide;
    end
    count = count+1;

    %time-domain plot

    %spectrogram
    %subplot(nrow,ncol,count+1);
    subplot(nrow,ncol,Ichan);
    overlap = 1-1/2;plots=0;
    [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan)*1e6,...
        N,overlap,fs,plots);
    wT = wT + (N/2)/fs;

    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    if (Ichan > 6); xlabel('Time (sec)');end
    ylabel('Frequency (Hz)');
    if (isempty(chan_names))
        title(['Channel ' num2str(Ichan)]);
    else
        title(chan_names{Ichan});
    end
    %set(gca,'Clim',-80+[-50 0]+10*log10(256)-10*log10(N));
    set(gca,'Clim',+20+[-35 0]+10*log10(256)-10*log10(N));
    ylim(f_lim);yl=ylim;
    xlim(t_plot_sec);
    %xlabel('Time (sec)');
    cl=get(gca,'Clim');
    colorbar
    clabel({'Intensity';'(dB/bin re: 1uV)'});
    ax(end+1)=gca;

    if ~isempty(t_mark_sec);
        for Imark=1:size(t_mark_sec,1);
            hold on;
            yl=ylim;
            plot(t_mark_sec(Imark,1)*[1 1],yl,'k--','linewidth',2);
            plot(t_mark_sec(Imark,2)*[1 1],yl,'k--','linewidth',2);
            hold off
        end
    end


    %get mean pD within the window
    for Iwin = 1:size(t_analyze_sec,1)
        %        inds=[];
        %       for Imark = 1:size(t_mark_sec,1)
        K=find((wT >= t_analyze_sec(Iwin,1)) & (wT <= t_analyze_sec(Iwin,2)));
        %             inds = [inds(:); K(:)];
        %         end
        mean_pD(:,Ichan,Iwin) = nanmedian(pD(:,K)')';

    end
end
clear Ichan
clear fftx ffty

% evaluate cross-channel coherence
nave = round(4*(1/(1 - overlap)));
%[mean_cohere,wT,f]=evalCoherence_nearbyOnly(t_sec,data_V,fs,N,overlap,nave,t_analyze_sec,f_lim);
subplot(3,1,3);plots=1;
[coherence,wT_cohere,f_cohere,mean_cohere] = calcCoherece_fromTimeDomain(t_sec,data_V,fs,N,overlap,nave,t_analyze_sec,plots);
ylim(f_lim);
xlim(t_plot_sec);
colorbar
clabel({'Mean Square';'Coherence'});
if isempty(chan_names);
    title({'EEG Coherence (Chan 1, Chan 2)';sname});
else
    title({['EEG Coherence (' chan_names{1} ' vs ' chan_names{2} ')'];sname});
end
xlabel('Time (sec)');

linkaxes(ax);


%% plot the mean pD and mean coherence
if ~isempty(t_analyze_sec)
    figure;setFigureTallWide;
    %c = get(gcf,'DefaultAxesColorOrder');
    % foo = c(5,:);c(5,:)=c(6,:);c(6,:)=foo;
    % c(end,:) = [0.5 0.5 0.5]; %lighten the gray
    % c(end+1,:) = [0 0 0]; %add black
    % c = c(end:-1:1,:);  %flip the order
    %set(gcf,'DefaultAxesColorOrder',c);

    %notch out 60
    notch_Hz = 60+3*[-1 1];
    I=find((f >= notch_Hz(1)) & (f <=notch_Hz(2)));
    mean_pD(I,:,:) = NaN;
    I=find((f_cohere >= notch_Hz(1)) & (f_cohere <=notch_Hz(2)));
    mean_cohere(I,:) = NaN;

    for Ichan=1:2
        subplot(2,2,Ichan);
        %flim=1;
        %I=find(f > flim); %plot only above 1Hz
        %plot(f(I(1):size(mean_pD,1)),sqrt(mean_pD(I(1):end,:))/1e-6,'linewidth',2);
        plot(f,sqrt(squeeze(mean_pD(:,Ichan,:))),'linewidth',2);

        xlabel(['Frequency (Hz)']);
        ylabel(['EEG Amplitude (uVrms per Bin)']);
        lt={};
        for I=1:size(mean_pD,3)
            lt{I} = ['[' num2str(round(t_analyze_sec(I,1))) '-' num2str(round(t_analyze_sec(I,2))) '] sec'];
        end
        if (~isempty(mark_names))
            h = legend(mark_names);
        else
            h=legend(lt,1);
        end
        if isempty(chan_names)
            title(['Channel ' num2str(Ichan)]);
        else
            title(chan_names{Ichan});
        end
        xlim(f_lim);
        ylim([0 1.4]);
        drawnow

        weaText({['fs = ' num2str(fs) ' Hz'];
            ['Nfft = ' num2str(N)]},3);
    end

    subplot(2,2,3);
    %I=find(f > flim); %plot only above 1Hz
    %hplots=plot(f(I(1):size(mean_cohere,1)),mean_cohere(I(1):end,:),'linewidth',2);
    hplots=plot(f_cohere,mean_cohere,'linewidth',2);
    xlabel(['Frequency (Hz)']);
    ylabel(['Coherence (MSC)']);

    if (~isempty(mark_names))
        h = legend(mark_names);
    else
        h=legend(lt,1);
    end
    moveLegendToSide(h);
    if isempty(chan_names);
        title({'EEG Coherence (Chan 1, Chan 2)';sname});
    else
        title({['EEG Coherence (' chan_names{1} ' vs ' chan_names{2} ')'];sname});
    end
    xlim(f_lim);ylim([0 1]);
    drawnow
    weaText({['fs = ' num2str(fs) ' Hz'];
        ['Nfft = ' num2str(N)]},3);
    %weaText(['Median [' num2str(t_analyze_sec(1)) ' ' num2str(t_analyze_sec(2)) '] sec'],3);

end