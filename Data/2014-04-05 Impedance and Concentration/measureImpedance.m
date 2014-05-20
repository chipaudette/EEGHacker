
%given_amp_counts = 4.5/2.4e-3;


pname = 'SavedData\';x_zoom_sec=[];
switch 3
    case 1
        %ECG Electrodes
        fname = 'openBCI_raw_2014-04-05_16-31-37_ECGelec_impedanceChecks_filt.txt';nchan=1;
        %t_lim_sec = [39 41.5; 46 48; 79 81;92 100; 138 144; 159 164; 178 184; 193 195;250 255 ; 263 267];
        t_lim_sec = [46 48; 55 58; 79 81;106 114; 140 148; 178 184; 193 195;250 255];
            x_zoom_sec = [100 155]-1;I=find((t_lim_sec(:,1) >= x_zoom_sec(1)) & (t_lim_sec(:,2) <= x_zoom_sec(2)));t_lim_sec = t_lim_sec(I,:);
     
        %t_lim_sec = [0 300];
    case 2
        %?? electrodes
        fname = 'openBCI_raw_2014-04-05_16-39-58.txt'; nchan=1;
        t_lim_sec = [21 26; 31.4 38];
    case 3
        %EEG Electrodes
        fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=1;
        t_lim_sec = [26 33; 42 44; 48.5 50.5; 60.5 62.5; 73 77; 108 115; 125 130; 139 144];
         x_zoom_sec = [37 117];I=find((t_lim_sec(:,1) >= x_zoom_sec(1)) & (t_lim_sec(:,2) <= x_zoom_sec(2)));t_lim_sec = t_lim_sec(I,:);
       
end

%fname = 'openBCI_raw_2014-04-05_16-36-34_ECGelec_countBackBy3_noFilt.txt';nchan=2;
%fname = 'openBCI_raw_2014-04-05_17-13-48_GoldCup_countBackBy3_afterLastAlpa.txt';nchan=2;
scale_fac_volts_count=2.23e-8;




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
bp_Hz = 32+[-5 5];
[b,a]=butter(3,bp_Hz/(fs/2));
Nfir = 2*round(0.5*fs);  %ensure an even number
[b,a]=weaFIR(Nfir,(32+[-5 5])/(fs/2));
fdata_V = filter(b,a,data_V);
fdata_V = [fdata_V(Nfir/2+1:end,:);zeros(Nfir/2,size(fdata_V,2))];  %remove latency
[b,a]=butter(3,[55 65]/(fs/2),'stop');
fdata_V = filter(b,a,fdata_V);

Nave = fs;
b = 1/Nave*ones(Nave,1);
a = 1;
rms_V = sqrt(filter(b,a,fdata_V.^2));
rms_V = [rms_V(Nave/2+1:end,:);zeros(Nave/2,size(rms_V,2))];  %remove filter latency
current_A = 6e-9;
current_A_rms = (current_A)/sqrt(2);
imp_Ohm = rms_V / current_A_rms;

%% analyze data
mean_data_V = [];
std_data_V = [];
for Idata=1:size(t_lim_sec);
    inds = round(t_lim_sec(Idata,:)*fs);
    inds = [max([1 inds(1)]) min([size(fdata_V,1) inds(2)])];
    inds = [inds(1):inds(2)];
    mean_data_V(Idata,:) = mean(fdata_V(inds,:));
    std_data_V(Idata,:) = std(fdata_V(inds,:));
    imp_data_Ohm(Idata,:) =  std_data_V(Idata,:) / current_A_rms;
end


%% time domain plot

t_sec = ([1:size(data_V,1)]-1)/fs;
Ichan=1;
figure;setFigureTallWide;
max_n_plots = 6;
for Idata=1:min([size(t_lim_sec,1) max_n_plots]);
    subplot(2,ceil(max_n_plots/2),Idata);
    
    t_win_width_sec = 0.15;
    inds = round(mean(t_lim_sec(Idata,:))*fs)+round(fs*0.5*t_win_width_sec*[-1 1]);
    inds = [inds(1):inds(2)];
    if (0)
        %raw
        foo = data_V(inds,Ichan)*1e6;
        foo = foo - mean(foo);
        plot(t_sec(inds),foo,'.-','markersize',5,'linewidth',2);
        tt='Raw Data';
    else
        %filtered
        plot(t_sec(inds),fdata_V(inds,Ichan)*1e6,'.-','markersize',5,'linewidth',2);
        tt=['Filtered Data (' num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz)'];
    end
    xlabel('Time (sec)');
    ylabel(['Voltage (uV)']);
    %title(fname,'interpreter','none');
    title(tt);
    %xlim(t_sec([1 end]));
    xlim(t_sec([inds(1) inds(end)]))
    ylim(800*[-1 1]);
    h=weaText({[num2str(std_data_V(Idata,Ichan)*1e6,3) ' uV rms'];
        [num2str(current_A*1e9) ' nA'];
        [num2str(imp_data_Ohm(Idata,Ichan)/1000,3) ' kOhm']},4);
    set(h,'BackgroundColor','white');
    clear h

    for Idata=1:size(t_lim_sec,1);
        hold on;
        hold on;yl=ylim;plot(t_lim_sec(Idata,1)*[1 1],yl,'g--','linewidth',2);plot(t_lim_sec(Idata,2)*[1 1],yl,'r--','linewidth',2);hold off
        hold off
    end
end


%% freq domain and amplitude
figure;setFigureTallPartWide;ax=[];

%subplot(4,1,1);
N=512;overlap = 1-1/8;plots=0;
Ichan=1;
% [pD,wT,f]=windowedFFTPlot_spectragram(data_V(:,Ichan),N,overlap,fs,plots);
% wT = wT + (N/2)/fs;
% imagesc(wT,f,10*log10(pD));
% set(gca,'Ydir','normal');
% if (Idata > 6); xlabel('Time (sec)');end
% ylabel('Frequency (Hz)');
% title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
% set(gca,'Clim',-80+[-50 0]+10*log10(256)-10*log10(N));
% cl=get(gca,'Clim');set(gca,'Clim',cl+5);
% ylim([0 40]);
% if ~isempty(x_zoom_sec);xlim(x_zoom_sec);else;xlim(t_sec([1 end]));end;xl=xlim;
% ax(end+1)=gca;

subplot(3,1,1);
plot(t_sec,fdata_V*1e6,'linewidth',2);
ylabel({'Filtered Signal (uV)';[num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz']});
ylim(1000*[-1 1]);
if ~isempty(x_zoom_sec);xlim(x_zoom_sec);else;xlim(t_sec([1 end]));end;xl=xlim;
xlim(xl);
xlabel(['Time (sec)']);
title([fname ', Channel ' num2str(Ichan)],'interpreter','none');
ax(end+1)=gca;


subplot(3,1,2);
plot(t_sec,rms_V*1e6,'linewidth',2);
ylim([0 800]);
ylabel({'RMS Voltage (uV)';[num2str(bp_Hz(1)) '-' num2str(bp_Hz(2)) ' Hz']});
xlabel(['Time (sec)']);
xlim(xl);
ax(end+1)=gca;

subplot(3,1,3);
plot(t_sec,imp_Ohm/1000,'linewidth',2);
ylim([0 200]);
ylabel(['Impedance (kOhm)']);
%set(gca,'YTick',[0.1 1 10 100 1000],'YTickLabel',{'0.1' '1' '10' '100' '1000'});
xlabel(['Time (sec)']);
xlim(xl);
ax(end+1)=gca;


for Iplot=1:3
    subplot(3,1,Iplot);
    for Idata=1:size(t_lim_sec,1);
        hold on;
        hold on;yl=ylim;plot(t_lim_sec(Idata,1)*[1 1],yl,'g--','linewidth',2);plot(t_lim_sec(Idata,2)*[1 1],yl,'r--','linewidth',2);hold off
        hold off
        
        yl=ylim;
        h=[]
        if (Iplot==1)
           %h=text(mean(t_lim_sec(Idata,:)),yl(1)+0.05*diff(yl),{num2str(std_data_V(Idata,1)*1e6,3); 'uVrms'},'VerticalAlignment','bottom');
        elseif (Iplot==2)
            h=text(mean(t_lim_sec(Idata,:)),std_data_V(Idata)*1e6+0.1*diff(yl),{num2str(std_data_V(Idata,1)*1e6,3); 'uVrms'},'VerticalAlignment','bottom');
        elseif (Iplot==3)
            h=text(mean(t_lim_sec(Idata,:)),imp_data_Ohm(Idata)/1000+0.1*diff(yl),[num2str(imp_data_Ohm(Idata,1)/1000,3) 'K'],'VerticalAlignment','bottom');
        end
        if ~isempty(h); set(h,'HorizontalAlignment','center','backgroundcolor','white','fontweight','bold');end
    end
end

linkaxes(ax,'x');
