
%given_amp_counts = 4.5/2.4e-3;

notch60=1;notch70=0;notch120=0;
t_plot_sec = [];

pname = 'SavedData\';
visual_stim_dur_sec = 20.1;
n_visual_stim = 10;
t_start_data_sec = [];
switch 5
    case 2
        fname = '02-openBCI_raw_2014-04-30_22-14-48_fullRun_4blocks.txt';nchan=2;
        t_start_data_sec = 182.3+1-2*visual_stim_dur_sec;
    case 3
        fname = '03-openBCI_raw_2014-04-30_22-21-19_fullRun_4blocks.txt';nchan=2;
        t_start_data_sec = 55+1-2*visual_stim_dur_sec;
    case 4
        fname = '04-openBCI_raw_2014-04-30_22-26-10_fullRun_2blocks.txt';nchan=2;
        t_start_data_sec = 100-2*visual_stim_dur_sec;
    case 5
        fname = '05-openBCI_raw_2014-04-30_22-31-22_fullRun_1block.txt';nchan=2;
        t_start_data_sec = 94.5-3*visual_stim_dur_sec;
        n_visual_stim=9;
    case 8
        fname = '08-openBCI_raw_2014-05-03_15-18-03_PhotcellOnAux_Block1.txt';nchan=2;
        %t_start_data_sec = 94.5-3*visual_stim_dur_sec;
        t_start_data_sec = 60;
    case 9
        fname ='09-openBCI_raw_2014-05-03_18-11-03_2speed_3movies_WMV.txt';nchan=2;
    case 10
        fname = '10-openBCI_raw_2014-05-03_18-26-27_2speed_4movies.WMV.txt';nchan=2;
end

%t_extra_sec = visual_stim_dur_sec;
t_extra_sec = 0;
t_win = [0 n_visual_stim*visual_stim_dur_sec+2*t_extra_sec];

if ~isempty(t_start_data_sec)
    t_plot_sec = t_start_data_sec - t_extra_sec + t_win;
else
    t_start_data_sec = visual_stim_dur_sec;
end

scale_fac_volts_count=2.23e-8;




%% load data
data_uV = load([pname fname]);  %loads data as microvolts
if size(data_uV,2)==10
    %get aux channel
    aux_vals = data_uV(:,end);
else
    aux_vals = zeros(size(data_uV,1),1);
end
data_uV = data_uV(:,[1:nchan+1]);
%fs = data2.fs_Hz;
fs = 250;
count = data_uV(:,1);  %first column is a packet counter (though it's broken)
data_V = data_uV(:,2:end) * 1e-6; %other columns are data
clear data_uV;

%% filter data
data_V = data_V - ones(size(data_V,1),1)*mean(data_V);
if notch60
    [b,a]=butter(2,[56.5 63.5]/(fs/2),'stop');
    data_V = filter(b,a,data_V);  %apply notch filter
end
if notch120
    [b,a]=butter(2,[115 122]/(fs/2),'stop');
    data_V = filter(b,a,data_V);  %apply notch filter
end
if notch70
    [b,a]=butter(2,[66.5 73.5]/(fs/2),'stop');
    data_V = filter(b,a,data_V);  %apply notch filter
end
hp_Hz = 0.2;
[b,a]=butter(2,hp_Hz/(fs/2),'high');
data_V = filter(b,a,data_V);
all_freq_Hz = [];all_freq_uVrms=[];all_noise_uVrms=[];
%% write to WAV
% fs_dec = fs;foo_V = data_V;
% %foo_V = resample(data_V,1,2); fs_dec = fs / 2;  %decimate
% outfname = ['WAVs\' fname(1:end-4) '.wav'];
% disp(['writing to ' outfname]);
% wavwrite(foo_V(:,1:min([size(data_V,2) 2]))*1e6/500,fs_dec,16,outfname);
%

%% analyze data
% mean_data_V = mean(data_V);
% median_data_V = median(data_V);
% std_data_V = std(data_V);
% spread_data_V = diff(xpercentile(data_V,0.5+(0.68-0.5)*[-1 1]))/2;
% spread_data_V = median(spread_data_V)*ones(size(spread_data_V));

%% plot data
t_sec = ([1:size(data_V,1)]-1)/fs;
if isempty(t_plot_sec);t_plot_sec = t_sec([1 end]);end;
nrow = max([2 size(data_V,2)]); ncol=1;
ax=[];

figure;setFigureTallestWide;
for Ichan=1:size(data_V,2);

    %spectrogram
    subplot(nrow,ncol,Ichan);
    %subplot(nrow,ncol,Ichan);

    N=256*4;overlap = 1-1/16;plots=0;
    yl = [0 22];
    %    end
    [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan)*1e6,N,overlap,fs,plots);
    wT = wT + (N/2)/fs - t_start_data_sec;

    imagesc(wT,f,10*log10(pD));
    set(gca,'Ydir','normal');
    xlabel('Time (sec)');
    ylabel('Frequency (Hz)');
    title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
    set(gca,'Clim',+20+[-40 0]+10*log10(256)-10*log10(N));
    ylim(yl);
    xlim(t_plot_sec-t_start_data_sec);xl=xlim;
    xlim([0 min([wT(end) xl(2)])]);
    %colorbar
    %clabel('dB/bin re: 1uVrms');

    %ax(end+1)=gca;

    ave_pD=[];lt={};
    for Ifreq=1:n_visual_stim
        yl=ylim;
        hold on;
        t_start = (Ifreq-1)*visual_stim_dur_sec;
        plot(t_start*[1 1],yl,'w--','linewidth',2);
        if (Ifreq==n_visual_stim);plot((t_start+visual_stim_dur_sec)*[1 1],yl,'w--','linewidth',2);end
        hold off

        %get average spectra for each frequency
        I=find((wT > t_start) & (wT <= t_start+visual_stim_dur_sec));
        ave_pD(:,Ifreq) = mean(pD(:,I)')';
        lt{Ifreq}=[num2str(Ifreq) ' Hz Complete Cycle'];
    end

    cl=get(gca,'Clim');
    h=weaText({['N = ' num2str(N) ', fs = ' num2str(fs) ' Hz'];
        ['Clim = [' num2str(round(cl(1))) ' ' num2str(round(cl(2))) '] dB']},2);
    set(h,'BackgroundColor','white');
    %end

    %% vertical
    % figure;
    % setFigureTallestWidest;
    if Ichan==2
        for Ifreq=1:n_visual_stim
            if (0)
                subplotTightBorder(2,n_visual_stim,Ifreq+n_visual_stim);
                plot(sqrt(ave_pD(:,Ifreq)),f,'.-','linewidth',2,'Color',[0 0.5 0]);
                ylim([0 22]);
                xlim([0 4.5]);
                %h=legend(lt);
                %moveLegendToSide(h);
                if (Ifreq ==1);ylabel('Frequency (Hz)');end
                %xlabel({'Amplitude';'(uVrms per bin)'});
                xlabel('uVrms Per Bin');
                %title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
                title([num2str(Ifreq)]);
                xl=xlim;
                hold on;
            end
            c = [0 0 1;1 0 0];
            for Iharm=1:2
                %plot(xl,Ifreq*Iharm*[1 1],'k--','linewidth',2);

                %find closest bin
                [foo,J]=min(abs(f-Ifreq*Iharm));
                win_Hz = 0.25;
                targ_Hz = Ifreq*Iharm;
                inds = find(abs(f-targ_Hz) <= win_Hz);
                if isempty(J); inds = J; end

                %find biggest bin within the window
                [foo,K]=max(ave_pD(inds,Ifreq));

                %find the bigger neighboring bin
                if (ave_pD(inds(K)+1,Ifreq) > ave_pD(inds(K)-1,Ifreq))
                    inds = inds(K)+[0 1];
                else
                    inds = inds(K)+[-1 0];
                end

                %combine and get total uV value
                sum_n_bins = length(inds);
                val_uVrms = sqrt(sum(ave_pD(inds,Ifreq)));
                all_freq_Hz(Ifreq,Iharm) = mean(f(inds));
                all_freq_uVrms(Ifreq,Iharm) = val_uVrms;

                if (0)
                    plot(val_uVrms,f(J),'o','linewidth',2,'color',c(Iharm,:));
                end
            end
            %plot(f,sqrt(ave_pD(:,Ifreq)),'linewidth',2);
            hold off

            all_noise_freq_Hz = [0.25:0.125:20];
            for I = 1:length(all_noise_freq_Hz)
                %assess amplitude
                [foo,J]=min(abs(f-all_noise_freq_Hz(I)));
                inds = J-ceil(sum_n_bins/2)+[0:sum_n_bins-1];
                if (all_noise_freq_Hz(I)==Ifreq) | (all_noise_freq_Hz(I)==2*Ifreq) | (all_noise_freq_Hz(I)==3*Ifreq) | (all_noise_freq_Hz(I)==4*Ifreq) | (all_noise_freq_Hz(I)==5*Ifreq) | (all_noise_freq_Hz(I)==6*Ifreq)
                    all_noise_uVrms(I,Ifreq) = NaN;
                else
                    all_noise_uVrms(I,Ifreq) = sqrt(sum(ave_pD(inds,Ifreq)));
                end
            end
        end

    end
end

%get final assessment of noise
all_noise_uVrms = sqrt(nanmean((all_noise_uVrms').^2)');

%%
figure;setFigureTall;
subplot(2,1,1);
for Iharm=1:size(all_freq_Hz,2)
    I = find(all_freq_Hz(:,Iharm) > 0.5);
    plot(all_freq_Hz(I,Iharm),all_freq_uVrms(I,Iharm),'o-','color',c(Iharm,:),'linewidth',2);
    hold on;
end
plot(all_noise_freq_Hz,all_noise_uVrms,'k--','linewidth',2);
hold off;
xlabel('Frequency (Hz)');
ylabel('EEG Response (uVrms)');
title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
xlim([0 20]);
ylim([0 5]);
legend('White-White Frequency','White-Black Frequency','Background Activity');
h=weaText({['N = ' num2str(N) ', fs = ' num2str(fs) ' Hz'];
    ['Sum PSD over ' num2str(sum_n_bins) ' bins']},2);
set(h,'BackgroundColor','white');
